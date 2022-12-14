#include <iostream>
#include <eh/Exception.hpp>

#include <Commons/Gason.hpp>

int
main(/*int argc, char* argv[]*/)
{
  //std::string str = "{\"id\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\",\"tmax\":150,\"at\":1,\"device\":{\"dnt\":0,\"devicetype\":2,\"js\":1,\"ip\":\"109.237.107.39\",\"ua\":\"Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 YaBrowser/19.3.1.828 Yowser/2.5 Safari/537.36\",\"os\":\"Windows OS\",\"language\":\"ru\",\"geo\":{\"country\":\"RUS\"}},\"user\":{\"id\":\"\",\"ext\":{\"pageview_number\":1,\"consent\":\"\"}},\"ext\":{\"ad_types\":[\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\"],\"landing_types\":[\"g\",\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\",\"b\"],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"device\":{\"dnt\":0,\"geo\":{\"country\":\"RUS\"}},\"user\":{\"id\":\"\",\"ext\":{\"pageview_number\":1,\"consent\":\"\"}},\"ext\":{\"ad_types\":[\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\"],\"landing_types\":[\"g\",\"pg\",\"pg13\",\"r\",\"nc17\",\"nsfw\",\"b\"],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[],\"imgSizeId\":19,\"category\":0,\"informerId\":5437,\"subid\":\"5714919_0\"},\"site\":{\"id\":\"5714919\",\"domain\":\"push-srv\",\"name\":\"push-srv\",\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[]},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[1,2,3,4,5,6,8,9,10,14]},\"ext\":null}],\"source\":{\"pchain\":\"d4c29acad76ce94f:5714919\",\"ext\":{\"schain\":{\"complete\":1,\"nodes\":[{\"asi\":\"mgid.com\",\"sid\":\"246175\",\"rid\":\"2e229d99-9825-11ea-8fe1-e4434b2123d2\"}]}}}}";
  //std::string str = "{\"user\":{\"id\":\"\",\"ext\":{}},\"ext\":{\"ad_types\":[],\"landing_types\":[]},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";

  //std::string str = "{\"ext\":{},\"site\":{\"publisher\":{\"id\":\"246175\",\"name\":\"push-srv\"},\"ext\":{\"traffic_type\":\"Direct\"}},\"publisher\":{},\"cur\":[\"RUB\",\"UAH\",\"USD\"],\"bcat\":[\"IAB23\",\"IAB24\",\"IAB25-1\",\"IAB25-2\",\"IAB25-4\",\"IAB25-5\",\"IAB25-6\",\"IAB25-7\",\"IAB26\",\"IAB25-3\",\"IAB9-5\",\"IAB9-7\",\"IAB9-30\"],\"imp\":[{\"id\":\"1\",\"bidfloor\":0.1419779069767442,\"instl\":0,\"secure\":1,\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";
  //std::string str = "{\"ext\":{},\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1,\"title\":{\"len\":75}},{\"id\":2,\"required\":1,\"img\":{\"wmin\":492,\"hmin\":328,\"type\":3,\"mimes\":[\"image/jpeg\",\"image/jpg\",\"image/png\"]}},{\"id\":4,\"required\":0,\"data\":{\"type\":6}},{\"id\":5,\"required\":0,\"data\":{\"type\":7}},{\"id\":6,\"required\":0,\"data\":{\"type\":1}},{\"id\":7,\"required\":0,\"img\":{\"type\":1}}]}\",\"ver\":\"1.1\",\"battr\":[]},\"ext\":null}],\"source\":{\"ext\":{}}}";
  //std::string str = "{\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1,\"adunit\":2,\"plcmtcnt\":1,\"plcmttype\":500,\"assets\":[{\"id\":1,\"required\":1}]}\"}}]}";
  std::string str = "{\"imp\":[{\"native\":{\"request\":\"{\"ver\":\"1.1\",\"layout\":1}\"}}]}";

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
      json_parse_error(status) << "' at pos : ";
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

  return 0;
}

