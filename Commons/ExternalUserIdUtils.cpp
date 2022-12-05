#include "Base32.hpp"
#include "ExternalUserIdUtils.hpp"

namespace AdServer
{
namespace Commons
{
  namespace
  {
    void
    escape_char(
      std::string& escaped_str,
      const String::SubString& str,
      char escape_char,
      char to_escape_char)
    {
      escaped_str.reserve(str.size() * 2);
      for(auto ch_it = str.begin(); ch_it != str.end(); ++ch_it)
      {
        if(*ch_it == to_escape_char)
        {
          escaped_str.push_back(escape_char);
          escaped_str.push_back(*ch_it);
        }
        else
        {
          escaped_str.push_back(*ch_it);
        }
      }
    }

    void
    dns_div(
      std::string& dns_res,
      const String::SubString& src)
    {
      const unsigned int MAX_DNS_PART_LEN = 63;

      dns_res.clear();
      dns_res.reserve(src.size() + 2 * src.size() / MAX_DNS_PART_LEN + 2);

      unsigned int cur_block_len = 0;
      auto prev_it = src.begin();
      for(auto it = src.begin(); it != src.end(); ++it, ++cur_block_len)
      {
        if(cur_block_len >= MAX_DNS_PART_LEN - 1)
        {
          if(!dns_res.empty())
          {
            dns_res += ".";
          }
          dns_res += "x";
          dns_res.append(prev_it, it);
          prev_it = it;
          cur_block_len = 0;
        }
      }

      if(!dns_res.empty())
      {
        dns_res += ".";
      }
      dns_res += "x";
      dns_res.append(prev_it, src.end());
    }

    void
    dns_undiv(
      std::string& undoted_src,
      const String::SubString& enc_src)
    {
      undoted_src.clear();
      undoted_src.reserve(enc_src.size());

      auto prev_it = enc_src.begin();
      for(auto it = enc_src.begin(); it != enc_src.end(); )
      {
        if(*it == '.')
        {
          undoted_src.append(prev_it, it);
          prev_it = enc_src.end();
          // skip one char after dot
          if(++it == enc_src.end() || ++it == enc_src.end())
          {
            break;
          }

          prev_it = it;
        }
        else
        {
          ++it;
        }
      }

      undoted_src.append(prev_it, enc_src.end());
    }

    /*
    ExternalUserId
    div_src_external_user_id(const String::SubString& src)
    {
      String::SubString::SizeType pos = src.find('/');
      if(pos != String::SubString::NPOS)
      {
        return ExternalUserId(
          src.substr(0, pos),
          src.substr(pos + 1));
      }
      else
      {
        return ExternalUserId(std::string(), src);
      }
    }
    */
  }

  void
  dns_encode_external_user_ids(
    std::string& res,
    const ExternalUserIdArray& user_ids)
    noexcept
  {
    std::string union_str;
    for(auto user_id_it = user_ids.begin();
      user_id_it != user_ids.end(); ++user_id_it)
    {
      if(user_id_it != user_ids.begin())
      {
        union_str += ",";
      }

      const std::string& comb = *user_id_it;
      //std::string comb = user_id_it->source_id;
      //comb += "/";
      //comb += user_id_it->id;

      std::string escaped_comb;
      escape_char(escaped_comb, comb, ',', ',');

      union_str += escaped_comb;
    }

    std::string encoded_str;
    base32_encode(encoded_str, union_str);

    // div for parts <= 63 symbols and add x to begin
    dns_div(res, encoded_str);
  }

  void
  dns_decode_external_user_ids(
    ExternalUserIdArray& user_ids,
    const String::SubString& src)
    noexcept
  {
    if(!src.empty())
    {
      String::SubString enc_src = src.substr(1);
      std::string undoted_src;

      dns_undiv(undoted_src, enc_src);

      std::string bind_user_ids;
      AdServer::Commons::base32_decode(
        bind_user_ids,
        undoted_src);

      // div by ','

      {
        std::string cur_block;
        for(auto it = bind_user_ids.begin(); it != bind_user_ids.end(); ++it)
        {
          if(*it == ',')
          {
            ++it;

            if(it != bind_user_ids.end() && *it == ',')
            {
              // escaped ','
              cur_block.push_back(',');
            }
            else
            {
              if(!cur_block.empty())
              {
                user_ids.push_back(cur_block); //div_src_external_user_id(cur_block));
              }

              cur_block.clear();
              if(it != bind_user_ids.end())
              {
                cur_block.push_back(*it);
              }
            }
          }
          else
          {
            cur_block.push_back(*it);
          }
        }

        if(!cur_block.empty())
        {
          user_ids.push_back(cur_block); //div_src_external_user_id(cur_block));
        }
      }
    } // !src.empty()
  }
}
}

