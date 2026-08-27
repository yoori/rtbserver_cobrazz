#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <sys/resource.h>

namespace AdServer::Tests::Frontends
{
  inline constexpr std::string_view OPENRTB_REQUEST = R"JSON(
{"id":"96bd100fa3cd49de846c0b6452be3ace","imp":[{"id":"1","banner":{"w":300,"h":250,"mimes":["text/html","text/javascript","image/png","image/jpeg","image/gif"]},"instl":0,"tagid":"2843845655","bidfloor":0.02,"bidfloorcur":"RUB","pmp":{"deals":[{"id":"40004_18-24","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"40005_25-34","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"40006_35-44","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42282_Образование","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42287_Развлечения_и_досуг","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42322_Семья_и_дети","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42335_Телеком","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42339_Еда_и_напитки","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42346_Финансы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42360_Строительство_обустройство_и_ремонт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42404_Отдых_и_путешествия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42422_Одежда_обувь_и_аксессуары","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42441_Бизнес","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42449_Транспорт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42485_Красота_и_здоровье","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42507_Работа","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42511_Электроника","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42533_Животные","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42546_Недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42559_Бытовая_техника","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42583_Спорт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42609_Подарки_и_цветы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42286_Образование_Дополнительное_образование_и_курсы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42293_Развлечения_и_досуг_Рестораны","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42294_Развлечения_и_досуг_Музеи","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42295_Развлечения_и_досуг_Кино","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42296_Развлечения_и_досуг_Кино_Билеты_в_кино","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42312_Развлечения_и_досуг_Театры","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42314_Развлечения_и_досуг_Музыка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42318_Развлечения_и_досуг_Рыбалка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42320_Развлечения_и_досуг_Книги","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42323_Семья_и_дети_Товары_для_детей","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42328_Семья_и_дети_Товары_для_детей_Детские_игрушки","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42340_Еда_и_напитки_Доставка_готовых_блюд","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42341_Еда_и_напитки_Доставка_воды","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42342_Еда_и_напитки_Кулинария","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42347_Финансы_Инвестиции","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42348_Финансы_Ипотека","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42349_Финансы_Финансовые_услуги_для_бизнеса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42353_Финансы_Банковские_вклады","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42354_Финансы_Форекс","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42355_Финансы_Кредитные_карты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42356_Финансы_Кредиты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42357_Финансы_Рефинансирование_кредитов","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42358_Финансы_Интернет_банкинг","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42363_Строительство_обустройство_и_ремонт_Дача_и_сад","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42367_Строительство_обустройство_и_ремонт_Мебель","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42370_Строительство_обустройство_и_ремонт_Мебель_Мебель_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42374_Строительство_обустройство_и_ремонт_Мебель_Мебель_для_детской","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42382_Строительство_обустройство_и_ремонт_Ремонт","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42397_Строительство_обустройство_и_ремонт_Товары_для_дома","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42406_Отдых_и_путешествия_Походы_и_спортивный_туризм","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42413_Отдых_и_путешествия_Авиабилеты","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42420_Отдых_и_путешествия_Билеты_на_поезд","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42423_Одежда_обувь_и_аксессуары_Обувь","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42428_Одежда_обувь_и_аксессуары_Аксессуары","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42429_Одежда_обувь_и_аксессуары_Аксессуары_Сумки_и_чемоданы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42435_Одежда_обувь_и_аксессуары_Одежда_Спортивная_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42436_Одежда_обувь_и_аксессуары_Одежда_Женская_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42437_Одежда_обувь_и_аксессуары_Одежда_Верхняя_одежда","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42443_Бизнес_Реклама","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42444_Бизнес_Юридическая_поддержка","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42445_Бизнес_Грузоперевозки_и_транспортные_услуги","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42446_Бизнес_Создание_и_продвижение_сайтов","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42448_Бизнес_Открытие_бизнеса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42467_Транспорт_Авто_Автомобили_представительского_класса","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42486_Красота_и_здоровье_Декоративная_косметика","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42487_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_лица","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42488_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_глаз","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42489_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_губ","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42490_Красота_и_здоровье_Декоративная_косметика_Декоративная_косметика_для_ногтей","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42491_Красота_и_здоровье_Уход_за_телом","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42492_Красота_и_здоровье_Парфюмерия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42494_Красота_и_здоровье_Парфюмерия_Женская_парфюмерия","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42495_Красота_и_здоровье_Уход_за_волосами","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42496_Красота_и_здоровье_Уход_за_волосами_Шампуни_для_волос","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42499_Красота_и_здоровье_Уход_за_волосами_Уход_после_мытья_волос","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42500_Красота_и_здоровье_Уход_за_лицом","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42508_Работа_Поиск_работы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42547_Недвижимость_Жилая_недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42549_Недвижимость_Аренда_недвижимости","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42556_Недвижимость_Загородная_недвижимость","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42560_Бытовая_техника_Техника_для_красоты_и_здоровья","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42562_Бытовая_техника_Техника_для_красоты_и_здоровья_Мужские_электробритвы","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42564_Бытовая_техника_Техника_для_красоты_и_здоровья_Устройства_для_ухода_за_полостью_рта","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42565_Бытовая_техника_Крупная_техника_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42572_Бытовая_техника_Мелкая_техника_для_кухни","bidfloor":0.02,"bidfloorcur":"RUB"},{"id":"42579_Бытовая_техника_Техника_для_дома","bidfloor":0.02,"bidfloorcur":"RUB"}]}}]}
)JSON";

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  inline std::uint64_t
  parse_uint64(const char* value, const char* option_name)
  {
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (end == value || *end != '\0' || parsed == 0)
    {
      throw std::runtime_error(std::string(option_name) + " must be positive integer");
    }
    return parsed;
  }

  inline std::string
  read_request_body(const std::string& file_path)
  {
    if (file_path.empty())
    {
      return std::string(OPENRTB_REQUEST);
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file)
    {
      throw std::runtime_error("can't open request file '" + file_path + "'");
    }

    std::ostringstream out;
    out << file.rdbuf();
    return out.str();
  }

  inline CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  inline std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }
}
