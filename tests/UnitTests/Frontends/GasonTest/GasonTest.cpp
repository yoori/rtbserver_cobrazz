#include <iostream>
#include <fstream>
#include <sstream>
#include <eh/Exception.hpp>

#include <Commons/Gason.hpp>

int
main(int argc, char* argv[])
{
  //std::string str = "{\"id\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\",\"tmax\":150,\"at\":1,\"device\":{\"dnt\":0,\"devicetype\":2,\"js\":1,\"ip\":\"109.237.107.39\",\"ua\":\"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 YaBrowser/19.3.1.828 Yowser/2.5 Safari/537.36\",\"os\":\"Windows OS\",\"language\":\"ru\",\"geo\":{\"country\":\"RUS\"}},\"user\":{\"id\":\"\",\"ext\":{\"pageview_number\":1,\"consent\":\"\"}},\"ext\":{\"ad_types\":[\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\"],\"landing_types\":[\"g\",\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\",\"b\"],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"device\":{\"dnt\":0,\"geo\":{\"country\":\"RUS\"}},\"user\":{\"id\":\"\",\"ext\":{\"pageview_number\":1,\"consent\":\"\"}},\"ext\":{\"ad_types\":[\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\"],\"landing_types\":[\"g\",\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\",\"b\"],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[]},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[]},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";

  //std::string str = "{\"ext\":{},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";
  //std::string str = "{\"ext\":{},\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";
  //std::string str = "{\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1}]}\"}}]}";
  //std::string str = "{\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1}\"}}]}";
  std::ifstream f(argv[1]);
  std::stringstream buf;
  buf << f.rdbuf();
  std::string str = buf.str();
  std::cout << "@" << str << "@" << std::endl;
  //std::string str = R"({"ext":{"notifications":{"lurl":1,"nurl":1}},"id":"18413652003647177169","user":{"id":"10343082851225181973","ext":{"consent":""}},"at":1,"tmax":500,"imp":[{"bidfloor":0.0100000000,"displaymanager":"Yandex Mobile Ads SDK","id":"137","instl":0,"secure":1,"tagid":"134783-137","native":{"ver":"1.2","request":"{\"native\":{\"ver\":\"1.2\",\"aurlsupport\":0,\"privacy\":0,\"plcmtcnt\":1,\"assets\":[{\"required\":1,\"title\":{\"len\":140},\"id\":1},{\"required\":1,\"img\":{\"wmin\":150,\"type\":3,\"hmin\":150},\"id\":2},{\"required\":1,\"data\":{\"len\":25,\"type\":1},\"id\":4},{\"required\":0,\"data\":{\"len\":140,\"type\":2},\"id\":5},{\"required\":0,\"data\":{\"len\":10,\"type\":3},\"id\":6},{\"required\":0,\"data\":{\"len\":20,\"type\":4},\"id\":7},{\"required\":0,\"data\":{\"len\":20,\"type\":6},\"id\":8},{\"required\":0,\"data\":{\"len\":50,\"type\":11},\"id\":9},{\"required\":0,\"data\":{\"len\":15,\"type\":12},\"id\":10},{\"required\":0,\"img\":{\"wmin\":50,\"type\":1,\"hmin\":50},\"id\":11}],\"eventtrackers\":[{\"methods\":[1],\"event\":1}]}}"},"ext":{"unmoderated":0},"clickbrowser":1,"displaymanagerver":"5.3.0","bidfloorcur":"RUB"}],"source":{"fd":0,"ext":{"schain":{"ver":"1.0","nodes":[{"sid":"2412782","asi":"yandex.com","hp":1}],"complete":1}},"sourcetype":3,"tid":""},"cur":["RUB","USD","EUR","TRY","UAH","KZT","CHF"],"app":{"bundle":"com.avito.android","content":{"language":"ru"},"publisher":{"id":"2412782"},"id":"134783"},"device":{"lmt":0,"ua":"Dalvik/2.1.0 (Linux; U; Android 10; M2006C3MNG MIUI/V12.0.14.0.QCSRUXM)","ip":"77.232.165.173","ifa":"c27c3c9d-e843-4986-9b20-d949c442a575","mccmnc":"250-2","geo":{"country":"RUS","region":"RU-DA"},"dpidmd5":"","carrier":"MegaFon","dpidsha1":"","model":"M2006C3MNG","connectiontype":2,"make":"Xiaomi","os":"Android","ext":{"gaid":"c27c3c9d-e843-4986-9b20-d949c442a575"},"osv":"10","w":360,"devicetype":4,"h":725,"dnt":0,"language":"ru"},"regs":{"ext":{"gdpr":0},"coppa":0}})";

  JsonValue root_value;
  JsonAllocator json_allocator;
  Generics::ArrayAutoPtr<char> str_holder(str.size() + 1);
  ::strcpy(str_holder.get(), str.c_str());

  char* parse_end = str_holder.get();

  JsonParseStatus status = json_parse(
    str_holder.get(), &parse_end, &root_value, json_allocator);

  if(status != JSON_PARSE_OK)
  {
    Stream::Error ostr;
    ostr << "parsing error '" <<
      json_parse_error(status) << "' at pos (" << (parse_end - str_holder.get()) << "): ";
    if(parse_end)
    {
      ostr << std::string(parse_end, 20);
    }
    else
    {
      ostr << "null";
    }

    std::cerr << ostr.str() << std::endl;
  }
  else
  {
    std::cout << "parse success" << std::endl;
  }

  return 0;
}

