
#include "FrontendsPool.hpp"
#include <BiddingFrontend/BiddingFrontend.hpp>
#include <DirectoryModule/DirectoryModule.hpp>
#include <PubPixelFrontend/PubPixelFrontend.hpp>
#include <ContentFrontend/ContentFrontend.hpp>
#include <WebStatFrontend/WebStatFrontend.hpp>
#include <ActionFrontend/ActionFrontend.hpp>
#include <UserBindFrontend/UserBindFrontend.hpp>
#include <PassFrontend/PassFrontend.hpp>
#include <PassPixelFrontend/PassPixelFrontend.hpp>
#include <OptoutFrontend/OptoutFrontend.hpp>
#include <AdInstFrontend/AdInstFrontend.hpp>
#include <ClickFrontend/ClickFrontend.hpp>
#include <ImprTrackFrontend/ImprTrackFrontend.hpp>
#include <AdFrontend/AdFrontend.hpp>

namespace AdServer
{
  namespace Frontends
  {
    // FrontendsPool
    FrontendsPool::FrontendsPool(
      const char* config_path,
      const ModuleIdArray& modules,
      Logging::Logger* logger,
      StatHolder* stats,
      Generics::CompositeMetricsProvider* composite_metrics_provider)
      : config_(new Configuration(config_path)),
        modules_(modules),
        logger_(ReferenceCounting::add_ref(logger)),
        callback_(new Logging::ActiveObjectCallbackImpl(
          logger_,
          "FrontendsPool",
          "FrontendsPool",
          0)),
        stats_(ReferenceCounting::add_ref(stats)),
        composite_metrics_provider_(ReferenceCounting::add_ref(composite_metrics_provider)),
        common_module_(new CommonModule(logger_))
    {
      frontends_.reserve(4);
    }

    bool
    FrontendsPool::will_handle(const String::SubString&) noexcept
    {
      return true;
    }

    void
    FrontendsPool::handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept
    {
      for (auto frontend_it = frontends_.begin();
        frontend_it != frontends_.end(); frontend_it++)
      {
        if ((*frontend_it)->will_handle(request_holder->request().uri()))
        {
          (*frontend_it)->handle_request(
            std::move(request_holder),
            std::move(response_writer));
          return;
        }
      }

      FCGI::HttpResponse_var response(new FCGI::HttpResponse(1));
      response_writer->write(404, response);
    }

    void
    FrontendsPool::handle_request_noparams(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      /*throw(eh::Exception)*/
    {
      for (auto frontend_it = frontends_.begin();
           frontend_it != frontends_.end(); frontend_it++)
      {
        if ((*frontend_it)->will_handle(request_holder->request().uri()))
        {
          (*frontend_it)->handle_request_noparams(
            std::move(request_holder),
            std::move(response_writer));
          return;
        }
      }

      FCGI::HttpResponse_var response(new FCGI::HttpResponse(1));
      response_writer->write(404, response);
    }

    void
    FrontendsPool::init()
      /*throw(eh::Exception)*/
    {
      try
      {
        common_module_->set_config_file(config_->path().c_str());
        common_module_->init();
        config_->read();

        typedef Configuration::FeConfig Config;
        const Config& fe_config = config_->get();

        unsigned long request_threads = 0;
        auto use_threads = [&request_threads](unsigned long threads)
        {
          if (threads > request_threads)
          {
            request_threads = threads;
          }
        };

        for (auto module_it = modules_.begin(); module_it != modules_.end(); ++module_it)
        {
          if (*module_it == M_PUBPIXEL && fe_config.PubPixelFeConfiguration().present())
          {
            use_threads(fe_config.PubPixelFeConfiguration()->threads());
          }
          else if ((*module_it == M_CONTENT || *module_it == M_DIRECTORY) &&
            fe_config.ContentFeConfiguration().present())
          {
            use_threads(fe_config.ContentFeConfiguration()->threads());
          }
          else if (*module_it == M_WEBSTAT && fe_config.WebStatFeConfiguration().present())
          {
            use_threads(fe_config.WebStatFeConfiguration()->threads());
          }
          else if (*module_it == M_ACTION && fe_config.ActionFeConfiguration().present())
          {
            use_threads(fe_config.ActionFeConfiguration()->threads());
          }
          else if (*module_it == M_USERBIND && fe_config.UserBindFeConfiguration().present())
          {
            use_threads(fe_config.UserBindFeConfiguration()->threads());
          }
          else if (*module_it == M_PASSBACK && fe_config.PassFeConfiguration().present())
          {
            use_threads(fe_config.PassFeConfiguration()->threads());
          }
          else if (*module_it == M_PASSBACKPIXEL && fe_config.PassPixelFeConfiguration().present())
          {
            use_threads(fe_config.PassPixelFeConfiguration()->threads());
          }
          else if (*module_it == M_OPTOUT && fe_config.OptOutFeConfiguration().present())
          {
            use_threads(fe_config.OptOutFeConfiguration()->threads());
          }
          else if (*module_it == M_ADINST && fe_config.AdInstFeConfiguration().present())
          {
            use_threads(fe_config.AdInstFeConfiguration()->threads());
          }
          else if (*module_it == M_CLICK && fe_config.ClickFeConfiguration().present())
          {
            use_threads(fe_config.ClickFeConfiguration()->threads());
          }
          else if (*module_it == M_IMPRTRACK && fe_config.ImprTrackFeConfiguration().present())
          {
            use_threads(fe_config.ImprTrackFeConfiguration()->threads());
          }
          else if (*module_it == M_AD && fe_config.AdFeConfiguration().present())
          {
            use_threads(fe_config.AdFeConfiguration()->threads());
          }
        }

        if (request_threads != 0)
        {
          request_workers_ = std::make_shared<AdServer::Commons::ExecutorPool>(
            callback_,
            request_threads);
        }

        for(auto module_it = modules_.begin(); module_it != modules_.end(); ++module_it)
        {
          if(*module_it == M_BIDDING)
          {
            init_frontend<Bidding::Frontend>(
              fe_config.BidFeConfiguration(),
              logger_,
              common_module_,
              stats_,
              composite_metrics_provider_);
          }
          else if(*module_it == M_PUBPIXEL)
          {
            init_frontend<PubPixel::Frontend>(
              fe_config.PubPixelFeConfiguration(),
              logger_,
              request_workers_);
          }
          else if(*module_it == M_CONTENT)
          {
            init_frontend<ContentFrontend>(
              fe_config.ContentFeConfiguration(),
              logger_,
              request_workers_);
          }
          else if(*module_it == M_DIRECTORY)
          {
            init_frontend<DirectoryModule>(
              fe_config.ContentFeConfiguration(),
              logger_,
              request_workers_);
          }
          else if(*module_it == M_WEBSTAT)
          {
            init_frontend<WebStat::Frontend>(
              fe_config.WebStatFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_ACTION)
          {
            init_frontend<Action::Frontend>(
              fe_config.ActionFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_USERBIND)
          {
            init_frontend<UserBindFrontend>(
              fe_config.UserBindFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_PASSBACK)
          {
            init_frontend<Passback::Frontend>(
              fe_config.PassFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_PASSBACKPIXEL)
          {
            init_frontend<PassbackPixel::Frontend>(
              fe_config.PassPixelFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_OPTOUT)
          {
            init_frontend<OptoutFrontend>(
              fe_config.OptOutFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_ADINST)
          {
            init_frontend<Instantiate::Frontend>(
              fe_config.AdInstFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_CLICK)
          {
            init_frontend<ClickFrontend>(
              fe_config.ClickFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_IMPRTRACK)
          {
            init_frontend<ImprTrack::Frontend>(
              fe_config.ImprTrackFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_AD)
          {
            init_frontend<AdFrontend>(
              fe_config.AdFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
        }

        if (request_workers_)
        {
          request_workers_->activate_object();
        }
      }
      catch (const Configuration::InvalidConfiguration& ex)
      {
        Stream::Error ostr;
        ostr << "Invalid configuration: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    FrontendsPool::shutdown() noexcept
    {
      for (auto frontend_it = frontends_.begin();
           frontend_it != frontends_.end(); frontend_it++)
      {
        (*frontend_it)->shutdown();
      }

      frontends_.clear();

      if (request_workers_)
      {
        request_workers_->deactivate_object();
        request_workers_->wait_object();
        request_workers_.reset();
      }

      common_module_->shutdown();
    }

    template <class Frontend, typename Config, typename ...T>
    void
    FrontendsPool::init_frontend(
      const Config& cfg,
      T&&... params)
    {
      if (cfg.present())
      {
        frontends_.emplace_back(new Frontend(config_, std::forward<T>(params)...));
        frontends_.back()->init();
      }
    }
  }
}
