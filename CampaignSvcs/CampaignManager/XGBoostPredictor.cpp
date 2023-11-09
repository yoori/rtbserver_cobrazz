#define DISABLE_OPENMP

#include <CampaignSvcs/CampaignManager/Utils.hpp>

#include <fstream>
#include <rabit/internal/rabit-inl.h>
#include <xgboost/predictor.h>
#include <xgboost/learner.h>
#include <xgboost/data/simple_dmatrix.h>
#include <xgboost/data/adapter.h>
#include <xgboost/json_io.h>
#include <xgboost/gbm/gbtree_model.h>

#include <String/StringManip.hpp>

#undef DISABLE_OPENMP

//#include <Generics/MMap.hpp>
#include "XGBoostPredictor.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
namespace CTR
{
  namespace
  {
    const float DEFAULT_FEATURE_VALUE = -1.0;
  }

  namespace
  {
    const xgboost::Json*
    xgboost_json_find_attribute(const xgboost::Json& root, const std::string& attr_name)
    {
      const auto& value = root.GetValue();
      xgboost::Value::ValueKind kind = value.Type();
      if(kind == xgboost::Value::ValueKind::kObject)
      {
        const xgboost::JsonObject* json_object = dynamic_cast<const xgboost::JsonObject*>(&value);
        if(json_object)
        {
          const auto& attributes = json_object->GetObject();
          for(auto it = attributes.begin(); it != attributes.end(); ++it)
          {
            if(it->first == attr_name)
            {
              // found - return it
              return &(it->second);
            }
          }

          for(auto it = attributes.begin(); it != attributes.end(); ++it)
          {
            const xgboost::Json* found_node = xgboost_json_find_attribute(it->second, attr_name);
            if(found_node)
            {
              return found_node;
            }
          }
        }
      }

      return 0;
    }
  }

  // XGBoostPredictorPool::PredictorWrapperDescriptor
  struct XGBoostPredictorPool::PredictorWrapperDescriptor
  {
    PredictorWrapperDescriptor(const String::SubString& model_file);

    const std::string model_file_name;
    std::vector<unsigned char> model_buffer;
    std::unique_ptr<rabit::utils::MemoryFixSizeBuffer> model_stream;
  };

  // XGBoostPredictorPool::PredictorWrapper
  // thread unsafe wrapper for xgboost::learner::BoostLearner
  class XGBoostPredictorPool::PredictorWrapper //: public xgboost::Learner
  {
  public:
    PredictorWrapper(const PredictorWrapperDescriptor& init);

    virtual
    ~PredictorWrapper() noexcept;

    void
    init() noexcept;

    float
    predict(
      const HashArray& hashes,
      const HashArray* add_hashes1 = 0,
      const HashArray* add_hashes2 = 0,
      const HashArray* add_hashes3 = 0);

  protected:
    typedef xgboost::Learner PredictorImplType;
    //typedef xgboost::GradientBooster PredictorImplType;

  protected:
    //unsigned long feats_size_;
    float base_score_;
    // hold LearnerModelParam, because gbtree_model_ point to it
    std::unique_ptr<xgboost::LearnerModelParam> gbtree_learner_model_param_;
    std::unique_ptr<xgboost::gbm::GBTreeModel> gbtree_model_;
    std::unique_ptr<xgboost::Predictor> predictor_;
    std::vector<xgboost::bst_float> preds_;
    std::vector<xgboost::Entry> predict_features_;
  };

  // XGBoostPredictorPool::PredictorWrapperDescriptor impl
  XGBoostPredictorPool::
  PredictorWrapperDescriptor::PredictorWrapperDescriptor(
    const String::SubString& file)
    : model_file_name(file.str())
  {
    static const char* FUN = "XGBoostPredictorPool::"
      "PredictorWrapperDescriptor::PredictorWrapperDescriptor()";

    try
    {
      // force removing of mmapped region from RAM by OS:
      // copy it to internal buffer
      std::ifstream fstr(file.str().c_str(), std::ios::binary);
      fstr.unsetf(std::ios::skipws);

      fstr.seekg(0, std::ios::end);
      std::streampos file_size = fstr.tellg();
      fstr.seekg(0, std::ios::beg);

      model_buffer.resize(file_size);

      model_buffer.insert(model_buffer.begin(),
        std::istream_iterator<unsigned char>(fstr),
        std::istream_iterator<unsigned char>());

      model_stream.reset(
        new rabit::utils::MemoryFixSizeBuffer(
          &model_buffer[0], file_size));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open model file '" << file << "': " << ex.what();
      throw InvalidConfig(ostr.str());
    }
  }

  // XGBoostPredictorPool::PredictorWrapper impl
  XGBoostPredictorPool::
  PredictorWrapper::PredictorWrapper(const PredictorWrapperDescriptor& init)
    : //feats_size_(FEATURE_INDEX_MOD),
      base_score_(0.0)
  {
    //static const char* FUN = "XGBoostPredictorPool::PredictorWrapper::PredictorWrapper()";

    // load & resave model for prevent compatibility exception
    std::unique_ptr<xgboost::Learner> comp_learner(
      xgboost::Learner::Create(
        std::vector<std::shared_ptr<xgboost::DMatrix> >()));
    rabit::utils::MemoryFixSizeBuffer orig_model_stream_copy(*init.model_stream);
    comp_learner->LoadModel(&orig_model_stream_copy);

    // workaround for non full num feature set passed on training
    const std::map<std::string, std::string>& attrs = comp_learner->GetConfigurationArguments();

    unsigned long feats_size = 1;

    auto num_feature_attr_it = attrs.find("num_feature");
    if(num_feature_attr_it != attrs.end())
    {
      unsigned long model_num_feature;
      if(String::StringManip::str_to_int(num_feature_attr_it->second, model_num_feature))
      {
        feats_size = model_num_feature;
      }
    }

    xgboost::Json j_tree{xgboost::Object()};
    comp_learner->Configure(); // required, otherwise SaveModel throw exception
    comp_learner->SaveModel(&j_tree);

    /*
    {
      // DEBUG
      std::string json_str;
      xgboost::Json::Dump(j_tree, &json_str);
      std::cout << "===============" << std::endl << json_str << std::endl << "===============" << std::endl;
    }
    */

    const xgboost::Json* json_learner_model_param_node = xgboost_json_find_attribute(j_tree, "learner_model_param");
    if(json_learner_model_param_node)
    {
      const xgboost::Json* json_base_score = xgboost_json_find_attribute(*json_learner_model_param_node, "base_score");
      if(json_base_score)
      {
        if(json_base_score->GetValue().Type() == xgboost::Value::ValueKind::kNumber)
        {
          const xgboost::JsonNumber* json_val = dynamic_cast<const xgboost::JsonNumber*>(&json_base_score->GetValue());
          if(json_val)
          {
            base_score_ = json_val->GetNumber();
          }
        }
        else if(json_base_score->GetValue().Type() == xgboost::Value::ValueKind::kInteger)
        {
          const xgboost::JsonInteger* json_val = dynamic_cast<const xgboost::JsonInteger*>(&json_base_score->GetValue());
          if(json_val)
          {
            base_score_ = json_val->GetInteger();
          }
        }
        else if(json_base_score->GetValue().Type() == xgboost::Value::ValueKind::kString)
        {
          const xgboost::JsonString* json_val = dynamic_cast<const xgboost::JsonString*>(&json_base_score->GetValue());
          if(json_val)
          {
            try
            {
              base_score_ = std::stof(json_val->GetString());
            }
            catch(...)
            {}
          }
        }
      }
    }

    // now, init xgboost::gbm::GBTreeModel required for call Predictor::PredictInstance
    gbtree_learner_model_param_.reset(new xgboost::LearnerModelParam());
    gbtree_learner_model_param_->num_feature = feats_size;
    gbtree_learner_model_param_->base_score = base_score_;
    gbtree_learner_model_param_->num_output_group = 1;
    gbtree_model_.reset(new xgboost::gbm::GBTreeModel(&*gbtree_learner_model_param_));

    /*
    {
      // DEBUG
      xgboost::Json gbtree_json{xgboost::Object()};
      gbtree_model_->SaveModel(&gbtree_json);
      std::string json_str;
      xgboost::Json::Dump(gbtree_json, &json_str);
      std::cout << "===============" << std::endl << json_str << std::endl << "===============" << std::endl;
    }
    */

    // find "model" attribute and load from its content
    const xgboost::Json* json_model_node = xgboost_json_find_attribute(j_tree, "model");
    assert(json_model_node);
    gbtree_model_->LoadModel(*json_model_node);

    // create Predictor
    xgboost::GenericParameter generic_params;
    std::vector<std::pair<std::string, std::string> > args{
      { "gpu_id", "-1" },
      { "nthread", "1" }
    };
    generic_params.Init(args);

    predictor_.reset(xgboost::Predictor::Create("cpu_predictor", &generic_params));

    preds_.resize(1, 0.0);

    predict_features_.reserve(1024);
  }

  XGBoostPredictorPool::
  PredictorWrapper::~PredictorWrapper() noexcept
  {}

  void
  XGBoostPredictorPool::
  PredictorWrapper::init() noexcept
  {
    // do mock predict for initalize big buffers inside predictor
    predict(HashArray(), 0);
  }

  float
  XGBoostPredictorPool::
  PredictorWrapper::predict(
    const HashArray& features,
    const HashArray* add_features,
    const HashArray* add_features2,
    const HashArray* add_features3)
  {
    // fill features
    for(auto it = features.begin(); it != features.end(); ++it)
    {
      predict_features_.push_back(xgboost::Entry(it->first - 1, it->second));
    }

    const HashArray* add_f[] = { add_features, add_features2, add_features3 };

    for(unsigned int i = 0; i < sizeof(add_f) / sizeof(add_f[0]); ++i)
    {
      if(add_f[i])
      {
        for(auto it = add_f[i]->begin(); it != add_f[i]->end(); ++it)
        {
          predict_features_.push_back(xgboost::Entry(it->first - 1, it->second));
        }
      }
    }

    preds_[0] = 0.0; // this is base x that will be used
    xgboost::SparsePage::Inst predict_inst(&predict_features_[0], predict_features_.size());
    predictor_->PredictInstance(predict_inst, &preds_, *gbtree_model_);
    float result_x = *(preds_.begin());

    predict_features_.clear();

    //std::cerr << "result_x = " << result_x << std::endl;

    return (1 / (1 + std::exp(-result_x)));
  }

  // XGBoostPredictorPool::Predictor impl
  XGBoostPredictorPool::Predictor::Predictor(
    XGBoostPredictorPool_var pool,
    PredictorWrapperPtr&& predictor_impl)
    noexcept
    : pool_(std::move(pool)),
      predictor_impl_(std::move(predictor_impl))
  {}

  XGBoostPredictorPool::Predictor::~Predictor() noexcept
  {
    // return PredictorWrapper to pool
    pool_->release_(std::move(predictor_impl_));
  }

  float
  XGBoostPredictorPool::
  Predictor::predict(
    const HashArray& hashes,
    const HashArray* add_hashes1,
    const HashArray* add_hashes2,
    const HashArray* add_hashes3)
  {
    return predictor_impl_->predict(hashes, add_hashes1, add_hashes2, add_hashes3);
  }

  // XGBoostPredictorPool::XGBoostPredictorPool impl
  XGBoostPredictorPool::XGBoostPredictorPool(
    const String::SubString& model_file)
  {
    predictor_descriptor_.reset(new PredictorWrapperDescriptor(model_file));
    // create one PredictorWrapper (validate model)
    predictors_.push_back(
      PredictorWrapperPtr(new PredictorWrapper(*predictor_descriptor_)));
    (*predictors_.begin())->init();
  }

  XGBoostPredictorPool::~XGBoostPredictorPool()
    noexcept
  {}

  XGBoostPredictorPool::Predictor_var
  XGBoostPredictorPool::get_predictor()
    noexcept
  {
    PredictorWrapperList predictors;

    {
      SyncPolicy::WriteGuard lock(lock_);
      if(!predictors_.empty())
      {
        predictors.splice(predictors.begin(), predictors_, predictors_.begin());
      }
    }

    if(predictors.empty())
    {
      // create new learner
      predictors.push_back(
        PredictorWrapperPtr(new PredictorWrapper(*predictor_descriptor_)));
    }

    return XGBoostPredictorPool::Predictor_var(
      new Predictor(shared_from_this(), std::move(*predictors.begin())));
  }

  void
  XGBoostPredictorPool::release_(PredictorWrapperPtr&& predictor_impl)
    noexcept
  {
    PredictorWrapperList predictors;
    predictors.push_back(std::move(predictor_impl));

    SyncPolicy::WriteGuard lock(lock_);
    predictors_.splice(predictors_.begin(), predictors);
  }
}
}
}

