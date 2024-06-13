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
#include <EchoFrontend/EchoFrontend.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/HttpServer/HttpResponse.hpp>

namespace AdServer
{
  namespace Frontends
  {
    // FrontendsPool
    FrontendsPool::FrontendsPool(
      TaskProcessor& task_processor,
      const SchedulerPtr& scheduler,
      const char* config_path,
      const ModuleIdArray& modules,
      Logging::Logger* logger,
      StatHolder* stats,
      FrontendCommons::HttpResponseFactory* response_factory)
      : FrontendCommons::FrontendInterface(response_factory),
        task_processor_(task_processor),
        scheduler_(scheduler),
        config_(new Configuration(config_path)),
        modules_(modules),
        logger_(ReferenceCounting::add_ref(logger)),
        stats_(ReferenceCounting::add_ref(stats)),
        http_response_factory_(ReferenceCounting::add_ref(response_factory)),
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
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
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

      FrontendCommons::HttpResponse_var response = create_response();
      response_writer->write(404, response);
    }

    void
    FrontendsPool::handle_request_noparams(
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer)
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

      FrontendCommons::HttpResponse_var response = create_response();
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

        for(auto module_it = modules_.begin(); module_it != modules_.end(); ++module_it)
        {
          if(*module_it == M_BIDDING)
          {
            init_frontend<Bidding::Frontend>(
              fe_config.BidFeConfiguration(),
              logger_,
              common_module_,
              stats_,
              http_response_factory_.in());
          }
          else if(*module_it == M_PUBPIXEL)
          {
            init_frontend<PubPixel::Frontend>(
              fe_config.PubPixelFeConfiguration(),
              logger_,
              http_response_factory_.in());
          }
          else if(*module_it == M_CONTENT)
          {
            init_frontend<ContentFrontend>(
              fe_config.ContentFeConfiguration(),
              logger_,
              http_response_factory_.in());
          }
          else if(*module_it == M_DIRECTORY)
          {
            init_frontend<DirectoryModule>(
              fe_config.ContentFeConfiguration(),
              logger_,
              http_response_factory_.in());
          }
          else if(*module_it == M_WEBSTAT)
          {
            init_frontend<WebStat::Frontend>(
              fe_config.WebStatFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_ACTION)
          {
            init_frontend<Action::Frontend>(
              fe_config.ActionFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_USERBIND)
          {
            init_frontend<UserBindFrontend>(
              fe_config.UserBindFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_PASSBACK)
          {
            init_frontend<Passback::Frontend>(
              fe_config.PassFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_PASSBACKPIXEL)
          {
            init_frontend<PassbackPixel::Frontend>(
              fe_config.PassPixelFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_OPTOUT)
          {
            init_frontend<OptoutFrontend>(
              fe_config.OptOutFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_ADINST)
          {
            init_frontend<Instantiate::Frontend>(
              fe_config.AdInstFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_CLICK)
          {
            init_frontend<ClickFrontend>(
              fe_config.ClickFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_IMPRTRACK)
          {
            init_frontend<ImprTrack::Frontend>(
              fe_config.ImprTrackFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_AD)
          {
            init_frontend<AdFrontend>(
              fe_config.AdFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
          else if(*module_it == M_ECHO)
          {
            init_frontend<AdServer::Echo::Frontend>(
              fe_config.CommonFeConfiguration(),
              logger_,
              common_module_,
              http_response_factory_.in());
          }
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
        frontends_.emplace_back(
          new Frontend(
            task_processor_,
            scheduler_,
            config_,
            std::forward<T>(params)...));
        frontends_.back()->init();
      }
    }
  }
}

