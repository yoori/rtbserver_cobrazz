
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>

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
  namespace
  {
    constexpr unsigned long DEFAULT_BIDDING_INTERRUPT_THREADS = 10;

    const auto STARTUP_STARTED_AT = std::chrono::steady_clock::now();

    void
    trace_startup(const char* label)
    {
      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - STARTUP_STARTED_AT);
      std::cerr << "FCGI_STARTUP "
        << (elapsed.count() / 1000) << "."
        << (elapsed.count() % 1000) << " "
        << label << std::endl;
    }
  }

  namespace Frontends
  {
    // FrontendsPool
    FrontendsPool::FrontendsPool(
      const char* config_path,
      const ModuleIdArray& modules,
      Logging::Logger* logger,
      StatHolder* stats,
      Generics::CompositeMetricsProvider* composite_metrics_provider,
      unsigned long grpc_coalesce_threads,
      unsigned long service_index)
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
        service_index_(service_index),
        common_module_(new CommonModule(logger_))
    {
      frontends_.reserve(4);
      grpc_coalesce_runner_ =
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          callback_,
          std::make_shared<boost::asio::io_service>(),
          grpc_coalesce_threads);
      common_module_->set_grpc_coalesce_runner(grpc_coalesce_runner_);
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
        trace_startup("FrontendsPool common_module set_config begin");
        common_module_->set_config_file(config_->path().c_str());
        trace_startup("FrontendsPool common_module set_config end");

        trace_startup("FrontendsPool common_module add begin");
        add_child_object(common_module_.in());
        trace_startup("FrontendsPool common_module add end");

        trace_startup("FrontendsPool common_module init begin");
        common_module_->init();
        trace_startup("FrontendsPool common_module init end");
        trace_startup("FrontendsPool fe_config read begin");
        config_->read();
        trace_startup("FrontendsPool fe_config read end");

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
          else if (*module_it == M_BIDDING && fe_config.BidFeConfiguration().present())
          {
            use_threads(fe_config.BidFeConfiguration()->threads());
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
            request_threads,
            "bid-request");
        }

        unsigned long interrupt_threads = 0;
        for(const auto& module : modules_)
        {
          if(module == M_BIDDING && fe_config.BidFeConfiguration().present())
          {
            interrupt_threads =
              fe_config.BidFeConfiguration()->interrupt_threads();
            break;
          }
        }

        if(interrupt_threads == 0)
        {
          for(const auto& module : modules_)
          {
            if(module == M_BIDDING)
            {
              interrupt_threads = DEFAULT_BIDDING_INTERRUPT_THREADS;
              break;
            }
          }
        }

        if(interrupt_threads != 0)
        {
          timeout_workers_ = std::make_shared<AdServer::Commons::ExecutorPool>(
            callback_,
            interrupt_threads,
            "bid-timeout");
        }

        for(auto module_it = modules_.begin(); module_it != modules_.end(); ++module_it)
        {
          if(*module_it == M_BIDDING)
          {
            init_frontend<Bidding::Frontend>(
              "bidding",
              fe_config.BidFeConfiguration(),
              logger_,
              common_module_,
              stats_,
              composite_metrics_provider_,
              request_workers_,
              timeout_workers_,
              service_index_);
          }
          else if(*module_it == M_PUBPIXEL)
          {
            init_frontend<PubPixel::Frontend>(
              "pubpixel",
              fe_config.PubPixelFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_CONTENT)
          {
            init_frontend<ContentFrontend>(
              "content",
              fe_config.ContentFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_DIRECTORY)
          {
            init_frontend<DirectoryModule>(
              "directory",
              fe_config.ContentFeConfiguration(),
              logger_,
              request_workers_);
          }
          else if(*module_it == M_WEBSTAT)
          {
            init_frontend<WebStat::Frontend>(
              "webstat",
              fe_config.WebStatFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_ACTION)
          {
            init_frontend<Action::Frontend>(
              "action",
              fe_config.ActionFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_USERBIND)
          {
            init_frontend<UserBindFrontend>(
              "userbind",
              fe_config.UserBindFeConfiguration(),
              logger_,
              request_workers_,
              common_module_,
              stats_);
          }
          else if(*module_it == M_PASSBACK)
          {
            init_frontend<Passback::Frontend>(
              "passback",
              fe_config.PassFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_PASSBACKPIXEL)
          {
            init_frontend<PassbackPixel::Frontend>(
              "passbackpixel",
              fe_config.PassPixelFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_OPTOUT)
          {
            init_frontend<OptoutFrontend>(
              "optout",
              fe_config.OptOutFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_ADINST)
          {
            init_frontend<Instantiate::Frontend>(
              "adinst",
              fe_config.AdInstFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_CLICK)
          {
            init_frontend<ClickFrontend>(
              "click",
              fe_config.ClickFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_IMPRTRACK)
          {
            init_frontend<ImprTrack::Frontend>(
              "imprtrack",
              fe_config.ImprTrackFeConfiguration(),
              logger_,
              request_workers_,
              common_module_);
          }
          else if(*module_it == M_AD)
          {
            init_frontend<AdFrontend>(
              "ad",
              fe_config.AdFeConfiguration(),
              logger_,
              request_workers_,
              common_module_,
              composite_metrics_provider_);
          }
        }

        trace_startup("FrontendsPool grpc_coalesce_runner add begin");
        add_child_object(grpc_coalesce_runner_);
        trace_startup("FrontendsPool grpc_coalesce_runner add end");

        if (request_workers_)
        {
          trace_startup("FrontendsPool request_workers add begin");
          add_child_object(request_workers_);
          trace_startup("FrontendsPool request_workers add end");
        }

        if (timeout_workers_)
        {
          trace_startup("FrontendsPool timeout_workers add begin");
          add_child_object(timeout_workers_);
          trace_startup("FrontendsPool timeout_workers add end");
        }

        for (const auto& frontend : frontends_)
        {
          add_child_object(frontend.in());
        }

        init_frontends_();
      }
      catch (const Configuration::InvalidConfiguration& ex)
      {
        Stream::Error ostr;
        ostr << "Invalid configuration: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    FrontendsPool::init_frontends_()
    {
      std::vector<std::thread> threads;
      std::vector<std::exception_ptr> errors(frontends_.size());
      threads.reserve(frontends_.size());

      for (std::size_t i = 0; i < frontends_.size(); ++i)
      {
        FrontendCommons::Frontend_var frontend = frontends_[i];
        const std::string name = frontend_names_[i];
        threads.emplace_back([frontend, name, &errors, i]()
        {
          try
          {
            const std::string begin_label = std::string("FrontendsPool ") +
              name + " init begin";
            trace_startup(begin_label.c_str());
            frontend->init();
            const std::string end_label = std::string("FrontendsPool ") +
              name + " init end";
            trace_startup(end_label.c_str());
          }
          catch (...)
          {
            errors[i] = std::current_exception();
          }
        });
      }

      for (auto& thread : threads)
      {
        thread.join();
      }

      for (std::size_t i = 0; i < errors.size(); ++i)
      {
        if (!errors[i])
        {
          continue;
        }

        for (auto& frontend : frontends_)
        {
          frontend->deactivate_object();
          frontend->wait_object();
        }

        try
        {
          std::rethrow_exception(errors[i]);
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Frontend '" << frontend_names_[i] <<
            "' init failed: " << ex.what();
          throw Exception(ostr);
        }
        catch (...)
        {
          Stream::Error ostr;
          ostr << "Frontend '" << frontend_names_[i] <<
            "' init failed: unknown exception";
          throw Exception(ostr);
        }
      }
    }

    template <class Frontend, typename Config, typename ...T>
    void
    FrontendsPool::init_frontend(
      const char* name,
      const Config& cfg,
      T&&... params)
    {
      if (cfg.present())
      {
        frontend_names_.emplace_back(name);
        frontends_.emplace_back(new Frontend(config_, std::forward<T>(params)...));
      }
    }
  }
}
