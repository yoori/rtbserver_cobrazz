#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sys/resource.h>

#include <boost/asio.hpp>

#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserIdBlackList.hpp>
#include <xsd/CampaignSvcs/DomainConfig.hpp>

#define private public
#include <Frontends/CommonModule/CommonModule.hpp>
#undef private

#include <Frontends/Modules/BiddingFrontend/RequestInfoFiller.hpp>

namespace
{
  constexpr const char OPENRTB_REQUEST[] = R"JSON(
{"id":"96bd100fa3cd49de846c0b6452be3ace","imp":[{"id":"1","banner":{"w":300,"h":250,"mimes":["text/html","text/javascript","image/png","image/jpeg","image/gif"]},"instl":0,"tagid":"2843845655","bidfloor":0.02,"bidfloorcur":"RUB","pmp":{"deals":[{"id":"40004_18-24","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"40005_25-34","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"40006_35-44","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42282_Образование","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42287_Развлечения_и_досуг","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42322_Семья_и_дети","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42335_Телеком","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42339_Еда_и_напитки","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42346_Финансы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42360_Строительство_обустройство_и_ремонт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42404_Отдых_и_путешествия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42422_Одежда_обувь_и_аксессуары","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42441_Бизнес","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42449_Транспорт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42485_Красота_и_здоровье","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42507_Работа","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42511_Электроника","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42533_Животные","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42546_Недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42559_Бытовая_техника","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42583_Спорт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42609_Подарки_и_цветы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42286_Образование_Дополнительное_образование_и_курсы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42293_Развлечения_и_досуг_Рестораны","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42294_Развлечения_и_досуг_Музеи","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42295_Развлечения_и_досуг_Кино","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42296_Развлечения_и_досуг_Кино_Билеты_в_кино","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42312_Развлечения_и_досуг_Театры","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42314_Развлечения_и_досуг_Музыка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42318_Развлечения_и_досуг_Рыбалка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42320_Развлечения_и_досуг_Книги","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42323_Семья_и_дети_Товары_для_детей","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42328_Семья_и_дети_Товары_для_детей_Детские_игрушки","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42340_Еда_и_напитки_Доставка_готовых_блюд","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42341_Еда_и_напитки_Доставка_воды","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42342_Еда_и_напитки_Кулинария","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42347_Финансы_Инвестиции","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42348_Финансы_Ипотека","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42349_Финансы_Финансовые_услуги_для_бизнеса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42353_Финансы_Банковские_вклады","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42354_Финансы_Форекс","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42355_Финансы_Кредитные_карты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42356_Финансы_Кредиты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42357_Финансы_Рефинансирование_кредитов","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42358_Финансы_Интернет_банкинг","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42363_Строительство_обустройство_и_ремонт_Дача_и_сад","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42367_Строительство_обустройство_и_ремонт_Мебель","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42370_Строительство_обустройство_и_ремонт_Мебель_Мебель_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42374_Строительство_обустройство_и_ремонт_Мебель_Мебель_для_детской","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42382_Строительство_обустройство_и_ремонт_Ремонт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42397_Строительство_обустройство_и_ремонт_Товары_для_дома","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42406_Отдых_и_путешествия_Походы_и_спортивный_туризм","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42413_Отдых_и_путешествия_Авиабилеты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42420_Отдых_и_путешествия_Билеты_на_поезд","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42423_Одежда_обувь_и_аксессуары_Обувь","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42428_Одежда_обувь_и_аксессуары_Аксессуары","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42429_Одежда_обувь_и_аксессуары_Аксессуары_Сумки_и_чемоданы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42435_Одежда_обувь_и_аксессуары_Одежда_Спортивная_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42436_Одежда_обувь_и_аксессуары_Одежда_Женская_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42437_Одежда_обувь_и_аксессуары_Одежда_Верхняя_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42443_Бизнес_Реклама","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42444_Бизнес_Юридическая_поддержка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42445_Бизнес_Грузоперевозки_и_транспортные_услуги","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42446_Бизнес_Создание_и_продвижение_сайтов","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42448_Бизнес_Открытие_бизнеса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42467_Транспорт_Авто_Автомобили_представительского_класса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42486_Красота_и_здоровье_Декоративная_косметика","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42487_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_лица","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42488_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_глаз","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42489_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_губ","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42490_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_ногтей","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42491_Красота_и_здоровье_Уход_за_телом","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42492_Красота_и_здоровье_Парфюмерия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42494_Красота_и_здоровье_Парфюмерия_Женская_парфюмерия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42495_Красота_и_здоровье_Уход_за_волосами","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42496_Красота_и_здоровье_Уход_за_волосами_Шампуни_для_волос","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42499_Красота_и_здоровье_Уход_за_волосами_Уход_после_мытья_волос","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42500_Красота_и_здоровье_Уход_за_лицом","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42508_Работа_Поиск_работы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42547_Недвижимость_Жилая_недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42549_Недвижимость_Аренда_недвижимости","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42556_Недвижимость_Загородная_недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42560_Бытовая_техника_Техника_для_красоты_и_здоровья","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42562_Бытовая_техника_Техника_для_красоты_и_здоровья_Мужские_электробритвы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42564_Бытовая_техника_Техника_для_красоты_и_здоровья_Устройства_для_ухода_за_полостью_рта","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42565_Бытовая_техника_Крупная_техника_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42572_Бытовая_техника_Мелкая_техника_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42579_Бытовая_техника_Техника_для_дома","bidfloor":0.02,"bidfloorcur":"RUB"}]}}]}
)JSON";

  struct Options
  {
    std::uint64_t count = 1000000;
    std::string request_file;
    std::string fe_config;
    std::string domain_config;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: BiddingFrontendRequestInfoFillerPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>         fill_by_openrtb_request calls count (default: 1000000)\n"
      << "  --request-file <P>  file with OpenRTB request body\n"
      << "  --fe-config <P>     FeConfig.xml for UserIdConfig and domain_config_path\n"
      << "  --domain-config <P> domain config for referer parsing\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(1000000);
    StringOption opt_request_file;
    StringOption opt_fe_config;
    StringOption opt_domain_config;
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("request-file"), opt_request_file);
    args.add(equal_name("fe-config"), opt_fe_config);
    args.add(equal_name("domain-config"), opt_domain_config);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.request_file = *opt_request_file;
    options.fe_config = *opt_fe_config;
    options.domain_config = *opt_domain_config;

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    return options;
  }

  std::string
  read_request_body(const std::string& file_path)
  {
    if(file_path.empty())
    {
      return OPENRTB_REQUEST;
    }

    std::ifstream file(file_path, std::ios::binary);
    if(!file)
    {
      throw std::runtime_error("can't open request file '" + file_path + "'");
    }

    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
  }

  void
  init_common_module(
    AdServer::CommonModule& common_module,
    Logging::Logger* logger,
    const std::string& fe_config_path,
    const std::string& domain_config_path)
  {
    std::string actual_domain_config_path = domain_config_path;

    if(!fe_config_path.empty())
    {
      Config::ErrorHandler error_handler;
      std::unique_ptr<xsd::AdServer::Configuration::FeConfigurationType>
        fe_config(xsd::AdServer::Configuration::FeConfiguration(
          fe_config_path.c_str(),
          error_handler).release());

      if(error_handler.has_errors())
      {
        std::string error;
        throw std::runtime_error(error_handler.text(error));
      }

      if(!fe_config->CommonFeConfiguration().present())
      {
        throw std::runtime_error("CommonFeConfiguration not presented.");
      }

      const auto& common_config = fe_config->CommonFeConfiguration().get();
      const auto& user_id_config = common_config.UserIdConfig();

      AdServer::Commons::UserIdBlackList uid_blacklist;
      uid_blacklist.load(user_id_config, logger, "CommonModule");

      common_module.user_id_controller_ = new AdServer::UserIdController(
        user_id_config.public_key().c_str(),
        user_id_config.temp_public_key().c_str(),
        user_id_config.private_key().c_str(),
        user_id_config.ssp_public_key().c_str(),
        user_id_config.ssp_private_key().c_str(),
        user_id_config.ssp_uid_key(),
        user_id_config.cache_size(),
        user_id_config.temp_cache_size(),
        user_id_config.ssp_cache_size(),
        uid_blacklist);

      if(actual_domain_config_path.empty())
      {
        actual_domain_config_path = common_config.domain_config_path();
      }
    }

    if(actual_domain_config_path.empty())
    {
      return;
    }

    Config::ErrorHandler error_handler;
    std::unique_ptr<xsd::AdServer::Configuration::DomainConfigurationType>
      domain_config(xsd::AdServer::Configuration::DomainConfiguration(
        actual_domain_config_path.c_str(),
        error_handler).release());

    if(error_handler.has_errors())
    {
      std::string error;
      throw std::runtime_error(error_handler.text(error));
    }

    common_module.domain_parser_ =
      new AdServer::CampaignSvcs::DomainParser(*domain_config);
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if(getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }
}

extern "C"
{
  __attribute__((noinline))
  std::uint64_t
  bidding_request_info_filler_run_benchmark(
    const AdServer::Bidding::RequestInfoFiller& filler,
    std::uint64_t count,
    const std::string& request_body)
  {
    std::uint64_t checksum = 0;

    for(std::uint64_t i = 0; i < count; ++i)
    {
      AdServer::Bidding::CampaignManager::RequestParams request_params;
      AdServer::Bidding::RequestInfo request_info;
      AdServer::Bidding::JsonProcessingContext context;
      std::string keywords;

      request_info.current_time = Generics::Time::get_time_of_day();

      filler.fill_by_openrtb_request(
        request_params,
        request_info,
        keywords,
        context,
        request_body.c_str());

      checksum += request_params.ad_slots.size();
      checksum += keywords.size();
      checksum += request_info.bid_request_id.size();
      checksum += request_params.common_info.peer_ip.size();
    }

    return checksum;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const std::string request_body = read_request_body(options.request_file);

    Logging::Logger_var logger(new Logging::Null::Logger);
    AdServer::CommonModule_var common_module(new AdServer::CommonModule(logger));
    init_common_module(
      *common_module,
      logger,
      options.fe_config,
      options.domain_config);

    AdServer::Bidding::RequestInfoFiller::ExternalUserIdSet skip_external_ids;
    AdServer::Bidding::SourceMap sources;
    AdServer::Bidding::RequestInfoFiller::AccountTraitsById account_traits;

    const AdServer::Bidding::RequestInfoFiller filler(
      logger,
      1,
      common_module,
      std::shared_ptr<GeoIPMapping::IPMapCity2>(),
      "",
      skip_external_ids,
      false,
      "",
      sources,
      true,
      account_traits);

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    const std::uint64_t checksum = bidding_request_info_filler_run_benchmark(
      filler,
      options.count,
      request_body);

    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();

    const double elapsed = std::chrono::duration<double>(
      finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;
    const double rate = static_cast<double>(options.count) / elapsed;

    std::cout
      << "count=" << options.count << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rate_per_sec=" << format_float(rate) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << '\n'
      << "checksum=" << checksum << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
