#include <fstream>
#include <Logger/StreamLogger.hpp>
#include <Generics/AppUtils.hpp>

#include <Commons/Algs.hpp>
#include <CampaignSvcs/CampaignManager/CTRProvider.hpp>

using namespace AdServer::CampaignSvcs;

struct Holder
{
  std::list<Tag_var> tags;
  std::list<Colocation_var> colocations;
};

Creative_var
create_creative(
  Campaign_var& campaign,
  unsigned long account_id,
  unsigned long advertiser_id,
  unsigned long campaign_id,
  unsigned long ccg_id,
  unsigned long creative_id,
  unsigned long cc_id)
{
  Account_var account = new AccountDef();
  account->account_id = account_id;

  Account_var advertiser = new AccountDef();
  advertiser->account_id = advertiser_id;

  campaign = new Campaign();
  campaign->campaign_id = ccg_id;
  campaign->campaign_group_id = campaign_id;
  campaign->account = account;
  campaign->advertiser = advertiser;
  campaign->ctr = RevenueDecimal(0.999);

  Creative_var creative = new Creative(
    campaign,
    cc_id,
    creative_id,
    0,
    0,
    "",
    "",
    OptionValue(),
    "",
    "",
    Creative::CategorySet());

  return creative;
}

void
init_campaign_select_params(
  Holder& holder,
  CampaignSelectParams& campaign_select_params,
  unsigned long colo_id,
  unsigned long publisher_id,
  unsigned long tag_id,
  unsigned long size_id,
  const char* size_protocol_name)
{
  Account_var publisher = new AccountDef();
  publisher->account_id = publisher_id;

  Site_var site = new Site();
  site->site_id = 2;
  site->account = publisher;

  Tag_var tag = new Tag();
  tag->tag_id = tag_id;
  tag->site = site;
  tag->adjustment = RevenueDecimal(false, 1, 0);

  Size_var size = new Size();
  size->size_id = size_id;
  size->protocol_name = size_protocol_name;

  Tag::Size_var tag_size = new Tag::Size();
  tag_size->size = size;
  tag_size->max_text_creatives = 1;
  tag->sizes.insert(std::make_pair(size_id, tag_size));

  Account_var isp = new AccountDef();
  isp->account_id = 4;

  Colocation_var colocation = new Colocation();
  colocation->colo_id = colo_id;
  colocation->account = isp;

  holder.tags.push_back(tag);
  campaign_select_params.tag = tag;
  holder.colocations.push_back(colocation);
  campaign_select_params.colocation = colocation;
  campaign_select_params.tag_sizes = tag->sizes;
}

namespace MT
{
  struct Context
  {
    Campaign_var campaign;
    Creative_var creative;
    CampaignSelectParams_var request_params;

    CTRProvider_var ctr_provider;
    std::string model_path;
    //CTR::XGBoostPredictorPool_var xgboost_pool;
  };

  void*
  eval_ctr_thread(void* context_ptr)
  {
    Context* context = static_cast<Context*>(context_ptr);

    /*
    CTR::XGBoostPredictorPool_var xgboost_pool =
      new CTR::XGBoostPredictorPool(context->model_path);
    */
    for(int i = 0; i < 1; ++i)
    {
      //xgboost_pool->get_predictor();

      CTRProvider::Calculation_var calculation =
        context->ctr_provider->create_calculation(context->request_params);

      assert(calculation.in());

      CTRProvider::CalculationContext_var calculation_context =
        calculation->create_context(
          context->request_params->tag->sizes.begin()->second);

      RevenueDecimal ctr;
      for(int i = 0; i < 1; ++i)
      {
        ctr = calculation_context->get_ctr(context->creative);
      }
    }

    return 0;
  }
}

int main(int argc, char** argv) noexcept
{
  if(argc < 2)
  {
    return 0;
  }

  Generics::AppUtils::Args args(-1);

  Generics::AppUtils::Option<unsigned long> opt_threads(1000);

  args.add(
    Generics::AppUtils::equal_name("threads") ||
    Generics::AppUtils::short_name("t"),
    opt_threads);

  args.parse(argc - 1, argv + 1);
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.size() < 2)
  {
    std::cerr << "Command not defined" << std::endl;
    return -1;
  }

  Generics::AppUtils::Args::CommandList::const_iterator cmd_it = commands.begin();
  std::string command = *cmd_it;
  std::string config = *(++cmd_it);
  const int METER_NUM = 100000;

  try
  {
    Holder holder;

    if(command == "simple-test")
    {
      CTRProvider_var ctr_provider(new CTRProvider(config, Generics::Time::ZERO, nullptr));

      CampaignSelectParams_var request_params_ptr = new CampaignSelectParams(
        true, // profiling_available
        FreqCapIdSet(),
        SeqOrderMap(),
        0,
        0,
        Tag::SizeMap(),
        false,
        -1, // visibility
        -1 // viewability
        );

      CampaignSelectParams& request_params = *request_params_ptr;

      init_campaign_select_params(
        holder,
        request_params,
        1, // colo_id
        1, // publisher_id
        1,  // tag_id
        1,
        "protocol_name"
        );

      std::cout << "to create_calculation" << std::endl;

      CTRProvider::Calculation_var calculation =
        ctr_provider->create_calculation(request_params_ptr);

      if(calculation.in())
      {
        Campaign_var campaign;
        Creative_var creative = create_creative(campaign, 1, 1, 1, 1, 1, 1);
        CTRProvider::CalculationContext_var calculation_context =
          calculation->create_context(request_params.tag->sizes.begin()->second);
        RevenueDecimal ctr;
        for(int i = 0; i < METER_NUM; ++i)
        {
          ctr = calculation_context->get_ctr(creative);
        }
        std::cout << "algorithm '" << calculation->algorithm_id(creative) <<
          "': ctr = " << ctr << std::endl;
      }
      else
      {
        std::cout << "selected default ctr algorithm" << std::endl;
      }
    }
    else if(command == "thread-test")
    {
      std::unique_ptr<MT::Context> context(new MT::Context());

      context->request_params = new CampaignSelectParams(
        true, // profiling_available
        FreqCapIdSet(),
        SeqOrderMap(),
        0,
        0,
        Tag::SizeMap(),
        false,
        -1, // visibility
        -1 // viewability
        );

      init_campaign_select_params(
        holder,
        *(context->request_params),
        1, // colo_id
        1, // publisher_id
        1,  // tag_id
        1,
        "protocol_name"
        );

      context->creative = create_creative(context->campaign, 1, 1, 1, 1, 1, 1);

      std::cout << "to create_calculation" << std::endl;

      for(int k = 0; k < 1000; ++k)
      {
        std::cout << "iteration #" << k << std::endl;
        //context->model_path = config;
        //context->xgboost_pool = new CTR::XGBoostPredictorPool(config);
        context->ctr_provider = new CTRProvider(
          config, Generics::Time::ZERO, nullptr);

        std::vector<pthread_t> thread_ids;

        for(unsigned long i = 0; i < *opt_threads; ++i)
        {
          pthread_t tid;
          ::pthread_create(&tid, 0, &MT::eval_ctr_thread, context.get());
          thread_ids.push_back(tid);
        }

        for(std::vector<pthread_t>::reverse_iterator tit = thread_ids.rbegin();
          tit != thread_ids.rend(); ++tit)
        {
          ::pthread_join(*tit, 0);
        }
      }
    }
    else if(command == "csv")
    {
      CTRProvider_var ctr_provider(
        new CTRProvider(config, Generics::Time::ZERO, nullptr));

      if(cmd_it == commands.end())
      {
        std::cerr << "csv file not defined" << std::endl;
        return -1;
      }
      // parse csv
      std::fstream csv_file(*++cmd_it);
      if(!csv_file.is_open())
      {
        std::cerr << "Can't open csv file" << std::endl;
        return -1;
      }

      while(!csv_file.eof())
      {
        char line[32000];
        csv_file.getline(line, sizeof(line));
        if(line[0] == 0)
        {
          continue;
        }

        if(csv_file.bad())
        {
          std::cerr << "Can't read csv file" << std::endl;
          return -1;
        }

        CampaignSelectParams_var request_params_ptr = new CampaignSelectParams(
          true, // profiling_available
          FreqCapIdSet(),
          SeqOrderMap(),
          0,
          0,
          Tag::SizeMap(),
          false,
          -1, // visibility
          -1 // viewability
          );

        CampaignSelectParams& request_params = *request_params_ptr;

        unsigned long publisher_id;
        unsigned long tag_id;
        std::string size;
        unsigned long size_id;
        unsigned long advertiser_id = 1;
        unsigned long campaign_id;
        unsigned long creative_id;

        String::StringManip::Tokenizer tokenizer(
          String::SubString(line), ",");
        String::SubString token;
        /*
        tokenizer.get_token(token); // rid
        std::cout << "rid = " << token << std::endl;
        */
        /*
        tokenizer.get_token(token); // uid
        std::cout << "uid = " << token << std::endl;
        */

        // device,geoch,userch,domain,publisher,advertiser,tag,etag,campaign,bidts
        // clickts,size_id,creative_id
        {
          tokenizer.get_token(token); // device
          std::cout << "device = " << token << std::endl;
          if(!String::StringManip::str_to_int(token, request_params.last_platform_channel_id))
          {
            std::cerr << "invalid device: " << token << std::endl;
            return -1;
          }
        }

        {
          tokenizer.get_token(token); // geoch
          std::cout << "geo = " << token << std::endl;
          String::StringManip::Tokenizer sub_tokenizer(token, "|");
          String::SubString sub_token;
          while(sub_tokenizer.get_token(sub_token))
          {
            unsigned long channel_id;
            if(!String::StringManip::str_to_int(sub_token, channel_id))
            {
              std::cerr << "invalid geo channel: " << sub_token << std::endl;
              return -1;
            }

            request_params.geo_channels.insert(channel_id);
          }
        }

        {
          tokenizer.get_token(token); // userch
          std::cout << "userch = " << token << std::endl;
          String::StringManip::Tokenizer sub_tokenizer(token, "|");
          String::SubString sub_token;
          while(sub_tokenizer.get_token(sub_token))
          {
            unsigned long channel_id;
            if(!String::StringManip::str_to_int(sub_token, channel_id))
            {
              std::cerr << "invalid channel: " << sub_token << std::endl;
              return -1;
            }

            request_params.channels.insert(channel_id);
          }
        }

        {
          tokenizer.get_token(token); // domain
          request_params.referer_hostname = token.str();
          std::cout << "domain = " << request_params.referer_hostname << std::endl;
        }

        //tokenizer.get_token(token); // url

        {
          tokenizer.get_token(token); // publisher
          if(!String::StringManip::str_to_int(token, publisher_id))
          {
            std::cerr << "invalid publisher_id: " << token << std::endl;
            return -1;
          }
          std::cout << "publisher_id = " << publisher_id << std::endl;
        }

        {
          tokenizer.get_token(token); // advertiser
          if(!String::StringManip::str_to_int(token, advertiser_id))
          {
            std::cerr << "invalid advertiser_id: " << token << std::endl;
            return -1;
          }
          std::cout << "advertiser_id = " << advertiser_id << std::endl;
        }

        {
          tokenizer.get_token(token); // tag
          if(!String::StringManip::str_to_int(token, tag_id))
          {
            std::cerr << "invalid tag: " << token << std::endl;
            return -1;
          }
          std::cout << "tag_id = " << tag_id << std::endl;
        }

        {
          tokenizer.get_token(token); // etag
          request_params.ext_tag_id = token.str();
          if(request_params.ext_tag_id == "-" || request_params.ext_tag_id == "0")
          {
            request_params.ext_tag_id.clear();
          }
        }

        {
          tokenizer.get_token(token); // campaign
          if(!String::StringManip::str_to_int(token, campaign_id))
          {
            std::cerr << "invalid campaign: " << token << std::endl;
            return -1;
          }

          std::cout << "campaign_id = " << campaign_id << std::endl;
        }

        {
          unsigned long ts;
          tokenizer.get_token(token); // time
          if(!String::StringManip::str_to_int(token, ts))
          {
            std::cerr << "invalid time: " << token << std::endl;
            return -1;
          }

          request_params.time = Generics::Time(ts);
          request_params.time_hour = request_params.time.get_gm_time().tm_hour;
          request_params.time_week_day = ((
            request_params.time /
              Generics::Time::ONE_DAY.tv_sec).tv_sec + 3) % 7;

          std::cout << "time = " << request_params.time.gm_ft() <<
            ", hour = " << request_params.time_hour <<
            ", week_day = " << request_params.time_week_day << std::endl;
        }

        tokenizer.get_token(token); // clickts

        {
          tokenizer.get_token(token); // size_id
          if(!String::StringManip::str_to_int(token, size_id))
          {
            std::cerr << "invalid sizeid: " << token << std::endl;
            return -1;
          }

          std::cout << "size_id = " << size_id << std::endl;
        }

        creative_id = 0;

        /*
        {
          tokenizer.get_token(token); // creative_id
          if(!String::StringManip::str_to_int(token, creative_id))
          {
            std::cerr << "invalid creative id: " << token << std::endl;
            return -1;
          }

          std::cout << "creative_id = " << creative_id << std::endl;
        }
        */

        /*
        tokenizer.get_token(token); // ccid
        tokenizer.get_token(token); // sizetype
        tokenizer.get_token(token); // size
        size = token.str();
        std::cout << "size = " << size << std::endl;

        tokenizer.get_token(token); // bidfloor
        tokenizer.get_token(token); // bidprice
        tokenizer.get_token(token); // winprice
        */

        init_campaign_select_params(
          holder,
          request_params,
          1, // colo_id
          publisher_id, // publisher_id
          tag_id,  // tag_id
          size_id,
          size.c_str()
          );

        Campaign_var campaign;
        Creative_var creative = create_creative(
          campaign,
          1, // account_id
          advertiser_id,
          campaign_id,
          1,  // ccg_id
          creative_id,
          1
          );

        CTRProvider::Calculation_var calculation =
          ctr_provider->create_calculation(request_params_ptr);

        if(calculation.in())
        {
          CTRProvider::CalculationContext_var calculation_context =
            calculation->create_context(request_params.tag->sizes.begin()->second);
          RevenueDecimal ctr;
          for(int i = 0; i < METER_NUM; ++i)
          {
            ctr = calculation_context->get_ctr(creative);
          }
          std::cout << "algorithm '" << calculation->algorithm_id(creative) <<
            "': ctr = " << ctr << std::endl;
        }
        else
        {
          std::cout << "selected default ctr algorithm" << std::endl;
        }
      }
    }
  }
  catch(const CTRProvider::InvalidConfig& ex)
  {
    std::cerr << "Can't read config: " << ex.what() << std::endl;
  }

  return 0;
}
