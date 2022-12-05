

#include <sstream>

#include <tests/AutoTests/Commons/Common.hpp>
#include "Utils.hpp"
 
namespace AutoTest
{
  namespace ORM
  {
    namespace PQ
    {
      namespace DB = AutoTest::DBC;

      ORMObjectMember Account::members_[35] = {      
        { "account_manager_id", &(null<Account>()->account_manager_id), 0},        
        { "account_type_id", &(null<Account>()->account_type_id), 0},        
        { "adv_contact_id", &(null<Account>()->adv_contact_id), 0},        
        { "agency_account_id", &(null<Account>()->agency_account_id), 0},        
        { "billing_address_id", &(null<Account>()->billing_address_id), 0},        
        { "business_area", &(null<Account>()->business_area), 0},        
        { "cmp_contact_id", &(null<Account>()->cmp_contact_id), 0},        
        { "company_registration_number", &(null<Account>()->company_registration_number), 0},        
        { "contact_name", &(null<Account>()->contact_name), 0},        
        { "country_code", &(null<Account>()->country_code), 0},        
        { "currency_id", &(null<Account>()->currency), 0},        
        { "display_status_id", &(null<Account>()->display_status_id), 0},        
        { "flags", &(null<Account>()->flags), 0},        
        { "hid_profile", &(null<Account>()->hid_profile), 0},        
        { "internal_account_id", &(null<Account>()->internal_account_id), 0},        
        { "isp_contact_id", &(null<Account>()->isp_contact_id), 0},        
        { "last_deactivated", &(null<Account>()->last_deactivated), 0},        
        { "last_updated", &(null<Account>()->last_updated), 0},        
        { "legal_address_id", &(null<Account>()->legal_address_id), 0},        
        { "legal_name", &(null<Account>()->legal_name), 0},        
        { "message_sent", &(null<Account>()->message_sent), 0},        
        { "name", &(null<Account>()->name), 0},        
        { "notes", &(null<Account>()->notes), 0},        
        { "passback_below_fold", &(null<Account>()->passback_below_fold), 0},        
        { "pub_contact_id", &(null<Account>()->pub_contact_id), 0},        
        { "pub_pixel_optin", &(null<Account>()->pub_pixel_optin), 0},        
        { "pub_pixel_optout", &(null<Account>()->pub_pixel_optout), 0},        
        { "role_id", &(null<Account>()->role_id), 0},        
        { "specific_business_area", &(null<Account>()->specific_business_area), 0},        
        { "status", &(null<Account>()->status), 0},        
        { "text_adserving", &(null<Account>()->text_adserving), 0},        
        { "timezone_id", &(null<Account>()->timezone_id), 0},        
        { "use_pub_pixel", &(null<Account>()->use_pub_pixel), 0},        
        { "version", &(null<Account>()->version), 0},        
        { "account_id", &(null<Account>()->account_id_), 0},        
      };
      
      Account::Account (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Account::Account (DB::IConn& connection,      
        const ORMInt::value_type& account_id        
      )      
        :ORMObject<postgres_connection>(connection), account_id_(account_id)
      {}
      
      Account::Account(const Account& from)      
        :ORMObject<postgres_connection>(from), account_id_(from.account_id_),
        account_manager_id(from.account_manager_id),      
        account_type_id(from.account_type_id),      
        adv_contact_id(from.adv_contact_id),      
        agency_account_id(from.agency_account_id),      
        billing_address_id(from.billing_address_id),      
        business_area(from.business_area),      
        cmp_contact_id(from.cmp_contact_id),      
        company_registration_number(from.company_registration_number),      
        contact_name(from.contact_name),      
        country_code(from.country_code),      
        currency(from.currency),      
        display_status_id(from.display_status_id),      
        flags(from.flags),      
        hid_profile(from.hid_profile),      
        internal_account_id(from.internal_account_id),      
        isp_contact_id(from.isp_contact_id),      
        last_deactivated(from.last_deactivated),      
        last_updated(from.last_updated),      
        legal_address_id(from.legal_address_id),      
        legal_name(from.legal_name),      
        message_sent(from.message_sent),      
        name(from.name),      
        notes(from.notes),      
        passback_below_fold(from.passback_below_fold),      
        pub_contact_id(from.pub_contact_id),      
        pub_pixel_optin(from.pub_pixel_optin),      
        pub_pixel_optout(from.pub_pixel_optout),      
        role_id(from.role_id),      
        specific_business_area(from.specific_business_area),      
        status(from.status),      
        text_adserving(from.text_adserving),      
        timezone_id(from.timezone_id),      
        use_pub_pixel(from.use_pub_pixel),      
        version(from.version)      
      {      
      }
      
      Account& Account::operator=(const Account& from)      
      {      
        Unused(from);      
        account_id_ = from.account_id_;      
        account_manager_id = from.account_manager_id;      
        account_type_id = from.account_type_id;      
        adv_contact_id = from.adv_contact_id;      
        agency_account_id = from.agency_account_id;      
        billing_address_id = from.billing_address_id;      
        business_area = from.business_area;      
        cmp_contact_id = from.cmp_contact_id;      
        company_registration_number = from.company_registration_number;      
        contact_name = from.contact_name;      
        country_code = from.country_code;      
        currency = from.currency;      
        display_status_id = from.display_status_id;      
        flags = from.flags;      
        hid_profile = from.hid_profile;      
        internal_account_id = from.internal_account_id;      
        isp_contact_id = from.isp_contact_id;      
        last_deactivated = from.last_deactivated;      
        last_updated = from.last_updated;      
        legal_address_id = from.legal_address_id;      
        legal_name = from.legal_name;      
        message_sent = from.message_sent;      
        name = from.name;      
        notes = from.notes;      
        passback_below_fold = from.passback_below_fold;      
        pub_contact_id = from.pub_contact_id;      
        pub_pixel_optin = from.pub_pixel_optin;      
        pub_pixel_optout = from.pub_pixel_optout;      
        role_id = from.role_id;      
        specific_business_area = from.specific_business_area;      
        status = from.status;      
        text_adserving = from.text_adserving;      
        timezone_id = from.timezone_id;      
        use_pub_pixel = from.use_pub_pixel;      
        version = from.version;      
        return *this;      
      }
      
      bool Account::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE account SET VERSION = now()";      
        {      
          strm << " WHERE account_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Account::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_manager_id,"      
            "account_type_id,"      
            "adv_contact_id,"      
            "agency_account_id,"      
            "billing_address_id,"      
            "business_area,"      
            "cmp_contact_id,"      
            "company_registration_number,"      
            "contact_name,"      
            "country_code,"      
            "currency_id,"      
            "display_status_id,"      
            "flags,"      
            "hid_profile,"      
            "internal_account_id,"      
            "isp_contact_id,"      
            "last_deactivated,"      
            "last_updated,"      
            "legal_address_id,"      
            "legal_name,"      
            "message_sent,"      
            "name,"      
            "notes,"      
            "passback_below_fold,"      
            "pub_contact_id,"      
            "pub_pixel_optin,"      
            "pub_pixel_optout,"      
            "role_id,"      
            "specific_business_area,"      
            "status,"      
            "text_adserving,"      
            "timezone_id,"      
            "use_pub_pixel,"      
            "version "      
          "FROM account "      
          "WHERE account_id = :i1"));      
        query  << account_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 34; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Account::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE account SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 34; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE account_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 34; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Account::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (account_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('account_account_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> account_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO account (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 34; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " account_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 34; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 34; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Account::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM account WHERE account_id = :i1"));      
        query  << account_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Account::del ()      
      {      
        int status_id = get_display_status_id(conn, "account", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE account SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE account_id = :i1"));      
        query.set(status_id);      
        query  << account_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Account::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "account", status);      
        DB::Query  query(conn.get_query("UPDATE account SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE account_id = :i1"));      
        query.set(status_id);      
        query  << account_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Account::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Account" << std::endl;      
          out << "{" << std::endl;      
          out << "* account_id = " << strof(account_id_) << ";" << std::endl;      
          out << "  account_manager_id = " << strof(account_manager_id) << ";" << std::endl;      
          out << "  account_type_id = " << strof(account_type_id) << ";" << std::endl;      
          out << "  adv_contact_id = " << strof(adv_contact_id) << ";" << std::endl;      
          out << "  agency_account_id = " << strof(agency_account_id) << ";" << std::endl;      
          out << "  billing_address_id = " << strof(billing_address_id) << ";" << std::endl;      
          out << "  business_area = " << strof(business_area) << ";" << std::endl;      
          out << "  cmp_contact_id = " << strof(cmp_contact_id) << ";" << std::endl;      
          out << "  company_registration_number = " << strof(company_registration_number) << ";" << std::endl;      
          out << "  contact_name = " << strof(contact_name) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  currency = " << strof(currency) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  hid_profile = " << strof(hid_profile) << ";" << std::endl;      
          out << "  internal_account_id = " << strof(internal_account_id) << ";" << std::endl;      
          out << "  isp_contact_id = " << strof(isp_contact_id) << ";" << std::endl;      
          out << "  last_deactivated = " << strof(last_deactivated) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  legal_address_id = " << strof(legal_address_id) << ";" << std::endl;      
          out << "  legal_name = " << strof(legal_name) << ";" << std::endl;      
          out << "  message_sent = " << strof(message_sent) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  notes = " << strof(notes) << ";" << std::endl;      
          out << "  passback_below_fold = " << strof(passback_below_fold) << ";" << std::endl;      
          out << "  pub_contact_id = " << strof(pub_contact_id) << ";" << std::endl;      
          out << "  pub_pixel_optin = " << strof(pub_pixel_optin) << ";" << std::endl;      
          out << "  pub_pixel_optout = " << strof(pub_pixel_optout) << ";" << std::endl;      
          out << "  role_id = " << strof(role_id) << ";" << std::endl;      
          out << "  specific_business_area = " << strof(specific_business_area) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  text_adserving = " << strof(text_adserving) << ";" << std::endl;      
          out << "  timezone_id = " << strof(timezone_id) << ";" << std::endl;      
          out << "  use_pub_pixel = " << strof(use_pub_pixel) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Account because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Account& val)      
      {      
        out << "Account:" << std::endl;      
        out << " *account_id: " << val.account_id_ << std::endl;      
        out << "  account_manager_id: " << val.account_manager_id << std::endl;      
        out << "  account_type_id: " << val.account_type_id << std::endl;      
        out << "  adv_contact_id: " << val.adv_contact_id << std::endl;      
        out << "  agency_account_id: " << val.agency_account_id << std::endl;      
        out << "  billing_address_id: " << val.billing_address_id << std::endl;      
        out << "  business_area: " << val.business_area << std::endl;      
        out << "  cmp_contact_id: " << val.cmp_contact_id << std::endl;      
        out << "  company_registration_number: " << val.company_registration_number << std::endl;      
        out << "  contact_name: " << val.contact_name << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  currency: " << val.currency << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  hid_profile: " << val.hid_profile << std::endl;      
        out << "  internal_account_id: " << val.internal_account_id << std::endl;      
        out << "  isp_contact_id: " << val.isp_contact_id << std::endl;      
        out << "  last_deactivated: " << val.last_deactivated << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  legal_address_id: " << val.legal_address_id << std::endl;      
        out << "  legal_name: " << val.legal_name << std::endl;      
        out << "  message_sent: " << val.message_sent << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  notes: " << val.notes << std::endl;      
        out << "  passback_below_fold: " << val.passback_below_fold << std::endl;      
        out << "  pub_contact_id: " << val.pub_contact_id << std::endl;      
        out << "  pub_pixel_optin: " << val.pub_pixel_optin << std::endl;      
        out << "  pub_pixel_optout: " << val.pub_pixel_optout << std::endl;      
        out << "  role_id: " << val.role_id << std::endl;      
        out << "  specific_business_area: " << val.specific_business_area << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  text_adserving: " << val.text_adserving << std::endl;      
        out << "  timezone_id: " << val.timezone_id << std::endl;      
        out << "  use_pub_pixel: " << val.use_pub_pixel << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Accountaddress::members_[10] = {      
        { "city", &(null<Accountaddress>()->city), 0},        
        { "last_updated", &(null<Accountaddress>()->last_updated), 0},        
        { "line1", &(null<Accountaddress>()->line1), 0},        
        { "line2", &(null<Accountaddress>()->line2), 0},        
        { "line3", &(null<Accountaddress>()->line3), 0},        
        { "province", &(null<Accountaddress>()->province), 0},        
        { "state", &(null<Accountaddress>()->state), 0},        
        { "version", &(null<Accountaddress>()->version), 0},        
        { "zip", &(null<Accountaddress>()->zip), 0},        
        { "address_id", &(null<Accountaddress>()->address_id_), 0},        
      };
      
      Accountaddress::Accountaddress (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Accountaddress::Accountaddress (DB::IConn& connection,      
        const ORMInt::value_type& address_id        
      )      
        :ORMObject<postgres_connection>(connection), address_id_(address_id)
      {}
      
      Accountaddress::Accountaddress(const Accountaddress& from)      
        :ORMObject<postgres_connection>(from), address_id_(from.address_id_),
        city(from.city),      
        last_updated(from.last_updated),      
        line1(from.line1),      
        line2(from.line2),      
        line3(from.line3),      
        province(from.province),      
        state(from.state),      
        version(from.version),      
        zip(from.zip)      
      {      
      }
      
      Accountaddress& Accountaddress::operator=(const Accountaddress& from)      
      {      
        Unused(from);      
        address_id_ = from.address_id_;      
        city = from.city;      
        last_updated = from.last_updated;      
        line1 = from.line1;      
        line2 = from.line2;      
        line3 = from.line3;      
        province = from.province;      
        state = from.state;      
        version = from.version;      
        zip = from.zip;      
        return *this;      
      }
      
      bool Accountaddress::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE accountaddress SET VERSION = now()";      
        {      
          strm << " WHERE address_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << address_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountaddress::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "city,"      
            "last_updated,"      
            "line1,"      
            "line2,"      
            "line3,"      
            "province,"      
            "state,"      
            "version,"      
            "zip "      
          "FROM accountaddress "      
          "WHERE address_id = :i1"));      
        query  << address_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Accountaddress::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE accountaddress SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE address_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << address_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountaddress::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (address_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('accountaddress_address_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> address_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO accountaddress (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " address_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << address_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountaddress::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM accountaddress WHERE address_id = :i1"));      
        query  << address_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Accountaddress::del ()      
      {      
        return delet();      
      }
      
      void Accountaddress::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Accountaddress" << std::endl;      
          out << "{" << std::endl;      
          out << "* address_id = " << strof(address_id_) << ";" << std::endl;      
          out << "  city = " << strof(city) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  line1 = " << strof(line1) << ";" << std::endl;      
          out << "  line2 = " << strof(line2) << ";" << std::endl;      
          out << "  line3 = " << strof(line3) << ";" << std::endl;      
          out << "  province = " << strof(province) << ";" << std::endl;      
          out << "  state = " << strof(state) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Accountaddress because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Accountaddress& val)      
      {      
        out << "Accountaddress:" << std::endl;      
        out << " *address_id: " << val.address_id_ << std::endl;      
        out << "  city: " << val.city << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  line1: " << val.line1 << std::endl;      
        out << "  line2: " << val.line2 << std::endl;      
        out << "  line3: " << val.line3 << std::endl;      
        out << "  province: " << val.province << std::endl;      
        out << "  state: " << val.state << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  zip: " << val.zip << std::endl;      
        return out;      
      }      
      ORMObjectMember Accountfinancialdata::members_[15] = {      
        { "camp_credit_used", &(null<Accountfinancialdata>()->camp_credit_used), 0},        
        { "invoiced_outstanding", &(null<Accountfinancialdata>()->invoiced_outstanding), 0},        
        { "invoiced_received", &(null<Accountfinancialdata>()->invoiced_received), 0},        
        { "last_updated", &(null<Accountfinancialdata>()->last_updated), 0},        
        { "not_invoiced", &(null<Accountfinancialdata>()->not_invoiced), 0},        
        { "payments_billed", &(null<Accountfinancialdata>()->payments_billed), 0},        
        { "payments_paid", &(null<Accountfinancialdata>()->payments_paid), 0},        
        { "payments_unbilled", &(null<Accountfinancialdata>()->payments_unbilled), 0},        
        { "prepaid_amount", &(null<Accountfinancialdata>()->prepaid_amount), 0},        
        { "total_adv_amount", &(null<Accountfinancialdata>()->total_adv_amount), 0},        
        { "total_paid", &(null<Accountfinancialdata>()->total_paid), 0},        
        { "type", &(null<Accountfinancialdata>()->type), 0},        
        { "unbilled_schedule_of_works", &(null<Accountfinancialdata>()->unbilled_schedule_of_works), 0},        
        { "version", &(null<Accountfinancialdata>()->version), 0},        
        { "account_id", &(null<Accountfinancialdata>()->account_id_), 0},        
      };
      
      Accountfinancialdata::Accountfinancialdata (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Accountfinancialdata::Accountfinancialdata (DB::IConn& connection,      
        const ORMInt::value_type& account_id        
      )      
        :ORMObject<postgres_connection>(connection), account_id_(account_id)
      {}
      
      Accountfinancialdata::Accountfinancialdata(const Accountfinancialdata& from)      
        :ORMObject<postgres_connection>(from), account_id_(from.account_id_),
        camp_credit_used(from.camp_credit_used),      
        invoiced_outstanding(from.invoiced_outstanding),      
        invoiced_received(from.invoiced_received),      
        last_updated(from.last_updated),      
        not_invoiced(from.not_invoiced),      
        payments_billed(from.payments_billed),      
        payments_paid(from.payments_paid),      
        payments_unbilled(from.payments_unbilled),      
        prepaid_amount(from.prepaid_amount),      
        total_adv_amount(from.total_adv_amount),      
        total_paid(from.total_paid),      
        type(from.type),      
        unbilled_schedule_of_works(from.unbilled_schedule_of_works),      
        version(from.version)      
      {      
      }
      
      Accountfinancialdata& Accountfinancialdata::operator=(const Accountfinancialdata& from)      
      {      
        Unused(from);      
        camp_credit_used = from.camp_credit_used;      
        invoiced_outstanding = from.invoiced_outstanding;      
        invoiced_received = from.invoiced_received;      
        last_updated = from.last_updated;      
        not_invoiced = from.not_invoiced;      
        payments_billed = from.payments_billed;      
        payments_paid = from.payments_paid;      
        payments_unbilled = from.payments_unbilled;      
        prepaid_amount = from.prepaid_amount;      
        total_adv_amount = from.total_adv_amount;      
        total_paid = from.total_paid;      
        type = from.type;      
        unbilled_schedule_of_works = from.unbilled_schedule_of_works;      
        version = from.version;      
        return *this;      
      }
      
      bool Accountfinancialdata::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE accountfinancialdata SET VERSION = now()";      
        {      
          strm << " WHERE account_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountfinancialdata::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "camp_credit_used,"      
            "invoiced_outstanding,"      
            "invoiced_received,"      
            "last_updated,"      
            "not_invoiced,"      
            "payments_billed,"      
            "payments_paid,"      
            "payments_unbilled,"      
            "prepaid_amount,"      
            "total_adv_amount,"      
            "total_paid,"      
            "type,"      
            "unbilled_schedule_of_works,"      
            "version "      
          "FROM accountfinancialdata "      
          "WHERE account_id = :i1"));      
        query  << account_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 14; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Accountfinancialdata::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE accountfinancialdata SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 14; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE account_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 14; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountfinancialdata::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (account_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(account_id) + 1 FROM accountfinancialdata"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> account_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO accountfinancialdata (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 14; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " account_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 14; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 14; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << account_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountfinancialdata::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM accountfinancialdata WHERE account_id = :i1"));      
        query  << account_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Accountfinancialdata::del ()      
      {      
        return delet();      
      }
      
      void Accountfinancialdata::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Accountfinancialdata" << std::endl;      
          out << "{" << std::endl;      
          out << "* account_id = " << strof(account_id_) << ";" << std::endl;      
          out << "  camp_credit_used = " << strof(camp_credit_used) << ";" << std::endl;      
          out << "  invoiced_outstanding = " << strof(invoiced_outstanding) << ";" << std::endl;      
          out << "  invoiced_received = " << strof(invoiced_received) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  not_invoiced = " << strof(not_invoiced) << ";" << std::endl;      
          out << "  payments_billed = " << strof(payments_billed) << ";" << std::endl;      
          out << "  payments_paid = " << strof(payments_paid) << ";" << std::endl;      
          out << "  payments_unbilled = " << strof(payments_unbilled) << ";" << std::endl;      
          out << "  prepaid_amount = " << strof(prepaid_amount) << ";" << std::endl;      
          out << "  total_adv_amount = " << strof(total_adv_amount) << ";" << std::endl;      
          out << "  total_paid = " << strof(total_paid) << ";" << std::endl;      
          out << "  type = " << strof(type) << ";" << std::endl;      
          out << "  unbilled_schedule_of_works = " << strof(unbilled_schedule_of_works) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Accountfinancialdata because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Accountfinancialdata& val)      
      {      
        out << "Accountfinancialdata:" << std::endl;      
        out << " *account_id: " << val.account_id_ << std::endl;      
        out << "  camp_credit_used: " << val.camp_credit_used << std::endl;      
        out << "  invoiced_outstanding: " << val.invoiced_outstanding << std::endl;      
        out << "  invoiced_received: " << val.invoiced_received << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  not_invoiced: " << val.not_invoiced << std::endl;      
        out << "  payments_billed: " << val.payments_billed << std::endl;      
        out << "  payments_paid: " << val.payments_paid << std::endl;      
        out << "  payments_unbilled: " << val.payments_unbilled << std::endl;      
        out << "  prepaid_amount: " << val.prepaid_amount << std::endl;      
        out << "  total_adv_amount: " << val.total_adv_amount << std::endl;      
        out << "  total_paid: " << val.total_paid << std::endl;      
        out << "  type: " << val.type << std::endl;      
        out << "  unbilled_schedule_of_works: " << val.unbilled_schedule_of_works << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Accountrole::members_[3] = {      
        { "last_updated", &(null<Accountrole>()->last_updated), 0},        
        { "name", &(null<Accountrole>()->name), 0},        
        { "account_role_id", &(null<Accountrole>()->account_role_id_), 0},        
      };
      
      Accountrole::Accountrole (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Accountrole::Accountrole (DB::IConn& connection,      
        const ORMInt::value_type& account_role_id        
      )      
        :ORMObject<postgres_connection>(connection), account_role_id_(account_role_id)
      {}
      
      Accountrole::Accountrole(const Accountrole& from)      
        :ORMObject<postgres_connection>(from), account_role_id_(from.account_role_id_),
        last_updated(from.last_updated),      
        name(from.name)      
      {      
      }
      
      Accountrole& Accountrole::operator=(const Accountrole& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        name = from.name;      
        return *this;      
      }
      
      bool Accountrole::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "name "      
          "FROM accountrole "      
          "WHERE account_role_id = :i1"));      
        query  << account_role_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Accountrole::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE accountrole SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE account_role_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << account_role_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountrole::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (account_role_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(account_role_id) + 1 FROM accountrole"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> account_role_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO accountrole (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " account_role_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << account_role_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accountrole::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM accountrole WHERE account_role_id = :i1"));      
        query  << account_role_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Accountrole::del ()      
      {      
        return delet();      
      }
      
      void Accountrole::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Accountrole" << std::endl;      
          out << "{" << std::endl;      
          out << "* account_role_id = " << strof(account_role_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Accountrole because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Accountrole& val)      
      {      
        out << "Accountrole:" << std::endl;      
        out << " *account_role_id: " << val.account_role_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        return out;      
      }      
      ORMObjectMember Accounttype::members_[26] = {      
        { "account_role_id", &(null<Accounttype>()->account_role_id), 0},        
        { "adv_exclusion_approval", &(null<Accounttype>()->adv_exclusion_approval), 0},        
        { "adv_exclusions", &(null<Accounttype>()->adv_exclusions), 0},        
        { "auction_rate", &(null<Accounttype>()->auction_rate), 0},        
        { "campaign_check_1", &(null<Accounttype>()->campaign_check_1), 0},        
        { "campaign_check_2", &(null<Accounttype>()->campaign_check_2), 0},        
        { "campaign_check_3", &(null<Accounttype>()->campaign_check_3), 0},        
        { "campaign_check_on", &(null<Accounttype>()->campaign_check_on), 0},        
        { "channel_check_1", &(null<Accounttype>()->channel_check_1), 0},        
        { "channel_check_2", &(null<Accounttype>()->channel_check_2), 0},        
        { "channel_check_3", &(null<Accounttype>()->channel_check_3), 0},        
        { "channel_check_on", &(null<Accounttype>()->channel_check_on), 0},        
        { "flags", &(null<Accounttype>()->flags), 0},        
        { "io_management", &(null<Accounttype>()->io_management), 0},        
        { "last_updated", &(null<Accounttype>()->last_updated), 0},        
        { "max_keyword_length", &(null<Accounttype>()->max_keyword_length), 0},        
        { "max_keywords_per_channel", &(null<Accounttype>()->max_keywords_per_channel), 0},        
        { "max_keywords_per_group", &(null<Accounttype>()->max_keywords_per_group), 0},        
        { "max_url_length", &(null<Accounttype>()->max_url_length), 0},        
        { "max_urls_per_channel", &(null<Accounttype>()->max_urls_per_channel), 0},        
        { "mobile_operator_targeting", &(null<Accounttype>()->mobile_operator_targeting), 0},        
        { "name", &(null<Accounttype>()->name), 0},        
        { "show_browser_passback_tag", &(null<Accounttype>()->show_browser_passback_tag), 0},        
        { "show_iframe_tag", &(null<Accounttype>()->show_iframe_tag), 0},        
        { "version", &(null<Accounttype>()->version), 0},        
        { "account_type_id", &(null<Accounttype>()->account_type_id_), 0},        
      };
      
      Accounttype::Accounttype (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Accounttype::Accounttype (DB::IConn& connection,      
        const ORMInt::value_type& account_type_id        
      )      
        :ORMObject<postgres_connection>(connection), account_type_id_(account_type_id)
      {}
      
      Accounttype::Accounttype(const Accounttype& from)      
        :ORMObject<postgres_connection>(from), account_type_id_(from.account_type_id_),
        account_role_id(from.account_role_id),      
        adv_exclusion_approval(from.adv_exclusion_approval),      
        adv_exclusions(from.adv_exclusions),      
        auction_rate(from.auction_rate),      
        campaign_check_1(from.campaign_check_1),      
        campaign_check_2(from.campaign_check_2),      
        campaign_check_3(from.campaign_check_3),      
        campaign_check_on(from.campaign_check_on),      
        channel_check_1(from.channel_check_1),      
        channel_check_2(from.channel_check_2),      
        channel_check_3(from.channel_check_3),      
        channel_check_on(from.channel_check_on),      
        flags(from.flags),      
        io_management(from.io_management),      
        last_updated(from.last_updated),      
        max_keyword_length(from.max_keyword_length),      
        max_keywords_per_channel(from.max_keywords_per_channel),      
        max_keywords_per_group(from.max_keywords_per_group),      
        max_url_length(from.max_url_length),      
        max_urls_per_channel(from.max_urls_per_channel),      
        mobile_operator_targeting(from.mobile_operator_targeting),      
        name(from.name),      
        show_browser_passback_tag(from.show_browser_passback_tag),      
        show_iframe_tag(from.show_iframe_tag),      
        version(from.version)      
      {      
      }
      
      Accounttype& Accounttype::operator=(const Accounttype& from)      
      {      
        Unused(from);      
        account_type_id_ = from.account_type_id_;      
        account_role_id = from.account_role_id;      
        adv_exclusion_approval = from.adv_exclusion_approval;      
        adv_exclusions = from.adv_exclusions;      
        auction_rate = from.auction_rate;      
        campaign_check_1 = from.campaign_check_1;      
        campaign_check_2 = from.campaign_check_2;      
        campaign_check_3 = from.campaign_check_3;      
        campaign_check_on = from.campaign_check_on;      
        channel_check_1 = from.channel_check_1;      
        channel_check_2 = from.channel_check_2;      
        channel_check_3 = from.channel_check_3;      
        channel_check_on = from.channel_check_on;      
        flags = from.flags;      
        io_management = from.io_management;      
        last_updated = from.last_updated;      
        max_keyword_length = from.max_keyword_length;      
        max_keywords_per_channel = from.max_keywords_per_channel;      
        max_keywords_per_group = from.max_keywords_per_group;      
        max_url_length = from.max_url_length;      
        max_urls_per_channel = from.max_urls_per_channel;      
        mobile_operator_targeting = from.mobile_operator_targeting;      
        name = from.name;      
        show_browser_passback_tag = from.show_browser_passback_tag;      
        show_iframe_tag = from.show_iframe_tag;      
        version = from.version;      
        return *this;      
      }
      
      bool Accounttype::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE accounttype SET VERSION = now()";      
        {      
          strm << " WHERE account_type_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << account_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accounttype::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_role_id,"      
            "adv_exclusion_approval,"      
            "adv_exclusions,"      
            "auction_rate,"      
            "campaign_check_1,"      
            "campaign_check_2,"      
            "campaign_check_3,"      
            "campaign_check_on,"      
            "channel_check_1,"      
            "channel_check_2,"      
            "channel_check_3,"      
            "channel_check_on,"      
            "flags,"      
            "io_management,"      
            "last_updated,"      
            "max_keyword_length,"      
            "max_keywords_per_channel,"      
            "max_keywords_per_group,"      
            "max_url_length,"      
            "max_urls_per_channel,"      
            "mobile_operator_targeting,"      
            "name,"      
            "show_browser_passback_tag,"      
            "show_iframe_tag,"      
            "version "      
          "FROM accounttype "      
          "WHERE account_type_id = :i1"));      
        query  << account_type_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 25; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Accounttype::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE accounttype SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 25; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE account_type_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 25; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << account_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accounttype::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (account_type_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('accounttype_account_type_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> account_type_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO accounttype (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 25; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " account_type_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 25; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 25; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << account_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Accounttype::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM accounttype WHERE account_type_id = :i1"));      
        query  << account_type_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Accounttype::del ()      
      {      
        return delet();      
      }
      
      void Accounttype::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Accounttype" << std::endl;      
          out << "{" << std::endl;      
          out << "* account_type_id = " << strof(account_type_id_) << ";" << std::endl;      
          out << "  account_role_id = " << strof(account_role_id) << ";" << std::endl;      
          out << "  adv_exclusion_approval = " << strof(adv_exclusion_approval) << ";" << std::endl;      
          out << "  adv_exclusions = " << strof(adv_exclusions) << ";" << std::endl;      
          out << "  auction_rate = " << strof(auction_rate) << ";" << std::endl;      
          out << "  campaign_check_1 = " << strof(campaign_check_1) << ";" << std::endl;      
          out << "  campaign_check_2 = " << strof(campaign_check_2) << ";" << std::endl;      
          out << "  campaign_check_3 = " << strof(campaign_check_3) << ";" << std::endl;      
          out << "  campaign_check_on = " << strof(campaign_check_on) << ";" << std::endl;      
          out << "  channel_check_1 = " << strof(channel_check_1) << ";" << std::endl;      
          out << "  channel_check_2 = " << strof(channel_check_2) << ";" << std::endl;      
          out << "  channel_check_3 = " << strof(channel_check_3) << ";" << std::endl;      
          out << "  channel_check_on = " << strof(channel_check_on) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  io_management = " << strof(io_management) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  max_keyword_length = " << strof(max_keyword_length) << ";" << std::endl;      
          out << "  max_keywords_per_channel = " << strof(max_keywords_per_channel) << ";" << std::endl;      
          out << "  max_keywords_per_group = " << strof(max_keywords_per_group) << ";" << std::endl;      
          out << "  max_url_length = " << strof(max_url_length) << ";" << std::endl;      
          out << "  max_urls_per_channel = " << strof(max_urls_per_channel) << ";" << std::endl;      
          out << "  mobile_operator_targeting = " << strof(mobile_operator_targeting) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  show_browser_passback_tag = " << strof(show_browser_passback_tag) << ";" << std::endl;      
          out << "  show_iframe_tag = " << strof(show_iframe_tag) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Accounttype because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Accounttype& val)      
      {      
        out << "Accounttype:" << std::endl;      
        out << " *account_type_id: " << val.account_type_id_ << std::endl;      
        out << "  account_role_id: " << val.account_role_id << std::endl;      
        out << "  adv_exclusion_approval: " << val.adv_exclusion_approval << std::endl;      
        out << "  adv_exclusions: " << val.adv_exclusions << std::endl;      
        out << "  auction_rate: " << val.auction_rate << std::endl;      
        out << "  campaign_check_1: " << val.campaign_check_1 << std::endl;      
        out << "  campaign_check_2: " << val.campaign_check_2 << std::endl;      
        out << "  campaign_check_3: " << val.campaign_check_3 << std::endl;      
        out << "  campaign_check_on: " << val.campaign_check_on << std::endl;      
        out << "  channel_check_1: " << val.channel_check_1 << std::endl;      
        out << "  channel_check_2: " << val.channel_check_2 << std::endl;      
        out << "  channel_check_3: " << val.channel_check_3 << std::endl;      
        out << "  channel_check_on: " << val.channel_check_on << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  io_management: " << val.io_management << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  max_keyword_length: " << val.max_keyword_length << std::endl;      
        out << "  max_keywords_per_channel: " << val.max_keywords_per_channel << std::endl;      
        out << "  max_keywords_per_group: " << val.max_keywords_per_group << std::endl;      
        out << "  max_url_length: " << val.max_url_length << std::endl;      
        out << "  max_urls_per_channel: " << val.max_urls_per_channel << std::endl;      
        out << "  mobile_operator_targeting: " << val.mobile_operator_targeting << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  show_browser_passback_tag: " << val.show_browser_passback_tag << std::endl;      
        out << "  show_iframe_tag: " << val.show_iframe_tag << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Action::members_[12] = {      
        { "account_id", &(null<Action>()->account_id), 0},        
        { "click_window", &(null<Action>()->click_window), "30"},        
        { "conv_category_id", &(null<Action>()->conv_category_id), "0"},        
        { "cur_value", &(null<Action>()->cur_value), "0"},        
        { "display_status_id", &(null<Action>()->display_status_id), 0},        
        { "imp_window", &(null<Action>()->imp_window), "7"},        
        { "last_updated", &(null<Action>()->last_updated), 0},        
        { "name", &(null<Action>()->name), 0},        
        { "status", &(null<Action>()->status), 0},        
        { "url", &(null<Action>()->url), 0},        
        { "version", &(null<Action>()->version), 0},        
        { "action_id", &(null<Action>()->action_id_), 0},        
      };
      
      Action::Action (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Action::Action (DB::IConn& connection,      
        const ORMInt::value_type& action_id        
      )      
        :ORMObject<postgres_connection>(connection), action_id_(action_id)
      {}
      
      Action::Action(const Action& from)      
        :ORMObject<postgres_connection>(from), action_id_(from.action_id_),
        account_id(from.account_id),      
        click_window(from.click_window),      
        conv_category_id(from.conv_category_id),      
        cur_value(from.cur_value),      
        display_status_id(from.display_status_id),      
        imp_window(from.imp_window),      
        last_updated(from.last_updated),      
        name(from.name),      
        status(from.status),      
        url(from.url),      
        version(from.version)      
      {      
      }
      
      Action& Action::operator=(const Action& from)      
      {      
        Unused(from);      
        action_id_ = from.action_id_;      
        account_id = from.account_id;      
        click_window = from.click_window;      
        conv_category_id = from.conv_category_id;      
        cur_value = from.cur_value;      
        display_status_id = from.display_status_id;      
        imp_window = from.imp_window;      
        last_updated = from.last_updated;      
        name = from.name;      
        status = from.status;      
        url = from.url;      
        version = from.version;      
        return *this;      
      }
      
      bool Action::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE action SET VERSION = now()";      
        {      
          strm << " WHERE action_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << action_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Action::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "click_window,"      
            "conv_category_id,"      
            "cur_value,"      
            "display_status_id,"      
            "imp_window,"      
            "last_updated,"      
            "name,"      
            "status,"      
            "url,"      
            "version "      
          "FROM action "      
          "WHERE action_id = :i1"));      
        query  << action_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Action::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE action SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE action_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << action_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Action::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (action_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('action_action_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> action_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO action (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " action_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << action_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Action::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM action WHERE action_id = :i1"));      
        query  << action_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Action::del ()      
      {      
        int status_id = get_display_status_id(conn, "action", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE action SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE action_id = :i1"));      
        query.set(status_id);      
        query  << action_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Action::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "action", status);      
        DB::Query  query(conn.get_query("UPDATE action SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE action_id = :i1"));      
        query.set(status_id);      
        query  << action_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Action::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Action" << std::endl;      
          out << "{" << std::endl;      
          out << "* action_id = " << strof(action_id_) << ";" << std::endl;      
          out << "  account_id = " << strof(account_id) << ";" << std::endl;      
          out << "  click_window = " << strof(click_window) << ";" << std::endl;      
          out << "  conv_category_id = " << strof(conv_category_id) << ";" << std::endl;      
          out << "  cur_value = " << strof(cur_value) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  imp_window = " << strof(imp_window) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  url = " << strof(url) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Action because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Action& val)      
      {      
        out << "Action:" << std::endl;      
        out << " *action_id: " << val.action_id_ << std::endl;      
        out << "  account_id: " << val.account_id << std::endl;      
        out << "  click_window: " << val.click_window << std::endl;      
        out << "  conv_category_id: " << val.conv_category_id << std::endl;      
        out << "  cur_value: " << val.cur_value << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  imp_window: " << val.imp_window << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  url: " << val.url << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Appformat::members_[5] = {      
        { "last_updated", &(null<Appformat>()->last_updated), 0},        
        { "mime_type", &(null<Appformat>()->mime_type), 0},        
        { "name", &(null<Appformat>()->name), 0},        
        { "version", &(null<Appformat>()->version), 0},        
        { "app_format_id", &(null<Appformat>()->app_format_id_), 0},        
      };
      
      Appformat::Appformat (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Appformat::Appformat (DB::IConn& connection,      
        const ORMInt::value_type& app_format_id        
      )      
        :ORMObject<postgres_connection>(connection), app_format_id_(app_format_id)
      {}
      
      Appformat::Appformat(const Appformat& from)      
        :ORMObject<postgres_connection>(from), app_format_id_(from.app_format_id_),
        last_updated(from.last_updated),      
        mime_type(from.mime_type),      
        name(from.name),      
        version(from.version)      
      {      
      }
      
      Appformat& Appformat::operator=(const Appformat& from)      
      {      
        Unused(from);      
        app_format_id_ = from.app_format_id_;      
        last_updated = from.last_updated;      
        mime_type = from.mime_type;      
        name = from.name;      
        version = from.version;      
        return *this;      
      }
      
      bool Appformat::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE appformat SET VERSION = now()";      
        {      
          strm << " WHERE app_format_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << app_format_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Appformat::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "mime_type,"      
            "name,"      
            "version "      
          "FROM appformat "      
          "WHERE app_format_id = :i1"));      
        query  << app_format_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Appformat::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE appformat SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE app_format_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << app_format_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Appformat::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (app_format_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('appformat_app_format_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> app_format_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO appformat (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " app_format_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << app_format_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Appformat::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM appformat WHERE app_format_id = :i1"));      
        query  << app_format_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Appformat::del ()      
      {      
        return delet();      
      }
      
      void Appformat::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Appformat" << std::endl;      
          out << "{" << std::endl;      
          out << "* app_format_id = " << strof(app_format_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  mime_type = " << strof(mime_type) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Appformat because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Appformat& val)      
      {      
        out << "Appformat:" << std::endl;      
        out << " *app_format_id: " << val.app_format_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  mime_type: " << val.mime_type << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember BehavioralParameters::members_[10] = {      
        { "behav_params_list_id", &(null<BehavioralParameters>()->behav_params_list_id), 0},        
        { "channel_id", &(null<BehavioralParameters>()->channel), 0},        
        { "last_updated", &(null<BehavioralParameters>()->last_updated), 0},        
        { "minimum_visits", &(null<BehavioralParameters>()->minimum_visits), 0},        
        { "time_from", &(null<BehavioralParameters>()->time_from), 0},        
        { "time_to", &(null<BehavioralParameters>()->time_to), 0},        
        { "trigger_type", &(null<BehavioralParameters>()->trigger_type), 0},        
        { "version", &(null<BehavioralParameters>()->version), 0},        
        { "weight", &(null<BehavioralParameters>()->weight), "1"},        
        { "behav_params_id", &(null<BehavioralParameters>()->behav_params_id_), 0},        
      };
      
      BehavioralParameters::BehavioralParameters (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      BehavioralParameters::BehavioralParameters (DB::IConn& connection,      
        const ORMInt::value_type& behav_params_id        
      )      
        :ORMObject<postgres_connection>(connection), behav_params_id_(behav_params_id)
      {}
      
      BehavioralParameters::BehavioralParameters(const BehavioralParameters& from)      
        :ORMObject<postgres_connection>(from), behav_params_id_(from.behav_params_id_),
        behav_params_list_id(from.behav_params_list_id),      
        channel(from.channel),      
        last_updated(from.last_updated),      
        minimum_visits(from.minimum_visits),      
        time_from(from.time_from),      
        time_to(from.time_to),      
        trigger_type(from.trigger_type),      
        version(from.version),      
        weight(from.weight)      
      {      
      }
      
      BehavioralParameters& BehavioralParameters::operator=(const BehavioralParameters& from)      
      {      
        Unused(from);      
        behav_params_id_ = from.behav_params_id_;      
        behav_params_list_id = from.behav_params_list_id;      
        channel = from.channel;      
        last_updated = from.last_updated;      
        minimum_visits = from.minimum_visits;      
        time_from = from.time_from;      
        time_to = from.time_to;      
        trigger_type = from.trigger_type;      
        version = from.version;      
        weight = from.weight;      
        return *this;      
      }
      
      bool BehavioralParameters::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE behavioralparameters SET VERSION = now()";      
        {      
          strm << " WHERE behav_params_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << behav_params_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "behav_params_list_id,"      
            "channel_id,"      
            "last_updated,"      
            "minimum_visits,"      
            "time_from,"      
            "time_to,"      
            "trigger_type,"      
            "version,"      
            "weight "      
          "FROM behavioralparameters "      
          "WHERE behav_params_id = :i1"));      
        query  << behav_params_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE behavioralparameters SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE behav_params_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << behav_params_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (behav_params_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('behavioralparameters_behav_params_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> behav_params_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO behavioralparameters (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " behav_params_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << behav_params_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM behavioralparameters WHERE behav_params_id = :i1"));      
        query  << behav_params_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool BehavioralParameters::del ()      
      {      
        return delet();      
      }
      
      void BehavioralParameters::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: BehavioralParameters" << std::endl;      
          out << "{" << std::endl;      
          out << "* behav_params_id = " << strof(behav_params_id_) << ";" << std::endl;      
          out << "  behav_params_list_id = " << strof(behav_params_list_id) << ";" << std::endl;      
          out << "  channel = " << strof(channel) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  minimum_visits = " << strof(minimum_visits) << ";" << std::endl;      
          out << "  time_from = " << strof(time_from) << ";" << std::endl;      
          out << "  time_to = " << strof(time_to) << ";" << std::endl;      
          out << "  trigger_type = " << strof(trigger_type) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log BehavioralParameters because not select");      
        }      
      }
      
      bool BehavioralParameters::has_channel (const ORMInt::value_type& channel)      
      {      
        Unused(channel);        
        this->channel = channel;        
        DB::Query query(conn.get_query("SELECT "      
            "behav_params_id "      
          "FROM behavioralparameters "      
          "WHERE channel_id = :i1"));      
        query.set(channel);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> this->behav_params_id_;      
          return true;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::select_channel (const ORMInt::value_type& channel)      
      {      
        Unused(channel);        
        this->channel = channel;        
        DB::Query query(conn.get_query("SELECT "      
            "behav_params_list_id,"      
            "last_updated,"      
            "minimum_visits,"      
            "time_from,"      
            "time_to,"      
            "trigger_type,"      
            "version,"      
            "weight,"      
            "behav_params_id "      
          "FROM behavioralparameters "      
          "WHERE channel_id = :i1"));      
        query.set(channel);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> behav_params_list_id      
                 >> last_updated      
                 >> minimum_visits      
                 >> time_from      
                 >> time_to      
                 >> trigger_type      
                 >> version      
                 >> weight      
                 >> behav_params_id_;      
          return true;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::update_channel (const ORMInt::value_type& channel, bool set_defaults )      
      {      
        Unused(channel);        
        Unused(set_defaults);        
        this->channel = channel;        
        std::ostringstream strm;      
        strm << "UPDATE behavioralparameters SET ";      
        int counter = 1;      
          counter = update_(strm, counter, this, members_[0], set_defaults);      
          counter = update_(strm, counter, this, members_[2], set_defaults);      
          counter = update_(strm, counter, this, members_[3], set_defaults);      
          counter = update_(strm, counter, this, members_[4], set_defaults);      
          counter = update_(strm, counter, this, members_[5], set_defaults);      
          counter = update_(strm, counter, this, members_[6], set_defaults);      
          counter = update_(strm, counter, this, members_[7], set_defaults);      
          counter = update_(strm, counter, this, members_[8], set_defaults);      
        if(counter > 1)      
        {      
          strm << " WHERE channel_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          setin_(query, behav_params_list_id);      
          setin_(query, last_updated);      
          setin_(query, minimum_visits);      
          setin_(query, time_from);      
          setin_(query, time_to);      
          setin_(query, trigger_type);      
          setin_(query, version);      
          setin_(query, weight);      
          query.set(channel);      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool BehavioralParameters::delet_channel (const ORMInt::value_type& channel)      
      {      
        Unused(channel);        
        this->channel = channel;        
        DB::Query  query(conn.get_query("DELETE FROM behavioralparameters WHERE channel_id = :i1"));      
        query.set(channel);      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool BehavioralParameters::del_channel (const ORMInt::value_type& channel)      
      {      
        Unused(channel);        
        return delet_channel(channel);      
      }
      
      std::ostream& operator<< (std::ostream& out, const BehavioralParameters& val)      
      {      
        out << "BehavioralParameters:" << std::endl;      
        out << " *behav_params_id: " << val.behav_params_id_ << std::endl;      
        out << "  behav_params_list_id: " << val.behav_params_list_id << std::endl;      
        out << "  channel: " << val.channel << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  minimum_visits: " << val.minimum_visits << std::endl;      
        out << "  time_from: " << val.time_from << std::endl;      
        out << "  time_to: " << val.time_to << std::endl;      
        out << "  trigger_type: " << val.trigger_type << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  weight: " << val.weight << std::endl;      
        return out;      
      }      
      ORMObjectMember Behavioralparameterslist::members_[5] = {      
        { "last_updated", &(null<Behavioralparameterslist>()->last_updated), 0},        
        { "name", &(null<Behavioralparameterslist>()->name), 0},        
        { "threshold", &(null<Behavioralparameterslist>()->threshold), 0},        
        { "version", &(null<Behavioralparameterslist>()->version), 0},        
        { "behav_params_list_id", &(null<Behavioralparameterslist>()->id_), 0},        
      };
      
      Behavioralparameterslist::Behavioralparameterslist (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Behavioralparameterslist::Behavioralparameterslist (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Behavioralparameterslist::Behavioralparameterslist(const Behavioralparameterslist& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        last_updated(from.last_updated),      
        name(from.name),      
        threshold(from.threshold),      
        version(from.version)      
      {      
      }
      
      Behavioralparameterslist& Behavioralparameterslist::operator=(const Behavioralparameterslist& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        last_updated = from.last_updated;      
        name = from.name;      
        threshold = from.threshold;      
        version = from.version;      
        return *this;      
      }
      
      bool Behavioralparameterslist::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE behavioralparameterslist SET VERSION = now()";      
        {      
          strm << " WHERE behav_params_list_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Behavioralparameterslist::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "name,"      
            "threshold,"      
            "version "      
          "FROM behavioralparameterslist "      
          "WHERE behav_params_list_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Behavioralparameterslist::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE behavioralparameterslist SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE behav_params_list_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Behavioralparameterslist::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('behavioralparameterslist_behav_params_list_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO behavioralparameterslist (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " behav_params_list_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Behavioralparameterslist::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM behavioralparameterslist WHERE behav_params_list_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Behavioralparameterslist::del ()      
      {      
        return delet();      
      }
      
      void Behavioralparameterslist::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Behavioralparameterslist" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  threshold = " << strof(threshold) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Behavioralparameterslist because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Behavioralparameterslist& val)      
      {      
        out << "Behavioralparameterslist:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  threshold: " << val.threshold << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Campaign::members_[23] = {      
        { "account_id", &(null<Campaign>()->account), 0},        
        { "bill_to_user_id", &(null<Campaign>()->bill_to_user), 0},        
        { "budget", &(null<Campaign>()->budget), 0},        
        { "budget_manual", &(null<Campaign>()->budget_manual), 0},        
        { "campaign_type", &(null<Campaign>()->campaign_type), "'D'"},        
        { "commission", &(null<Campaign>()->commission), "0"},        
        { "daily_budget", &(null<Campaign>()->daily_budget), 0},        
        { "date_end", &(null<Campaign>()->date_end), 0},        
        { "date_start", &(null<Campaign>()->date_start), "current_date - 1"},        
        { "delivery_pacing", &(null<Campaign>()->delivery_pacing), "'U'"},        
        { "display_status_id", &(null<Campaign>()->display_status_id), "1"},        
        { "flags", &(null<Campaign>()->flags), 0},        
        { "freq_cap_id", &(null<Campaign>()->freq_cap), 0},        
        { "last_deactivated", &(null<Campaign>()->last_deactivated), 0},        
        { "last_updated", &(null<Campaign>()->last_updated), 0},        
        { "marketplace", &(null<Campaign>()->marketplace), 0},        
        { "max_pub_share", &(null<Campaign>()->max_pub_share), "1"},        
        { "name", &(null<Campaign>()->name), 0},        
        { "sales_manager_id", &(null<Campaign>()->sales_manager_id), 0},        
        { "sold_to_user_id", &(null<Campaign>()->sold_to_user), 0},        
        { "status", &(null<Campaign>()->status), "'A'"},        
        { "version", &(null<Campaign>()->version), 0},        
        { "campaign_id", &(null<Campaign>()->id_), 0},        
      };
      
      Campaign::Campaign (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Campaign::Campaign (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Campaign::Campaign(const Campaign& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        account(from.account),      
        bill_to_user(from.bill_to_user),      
        budget(from.budget),      
        budget_manual(from.budget_manual),      
        campaign_type(from.campaign_type),      
        commission(from.commission),      
        daily_budget(from.daily_budget),      
        date_end(from.date_end),      
        date_start(from.date_start),      
        delivery_pacing(from.delivery_pacing),      
        display_status_id(from.display_status_id),      
        flags(from.flags),      
        freq_cap(from.freq_cap),      
        last_deactivated(from.last_deactivated),      
        last_updated(from.last_updated),      
        marketplace(from.marketplace),      
        max_pub_share(from.max_pub_share),      
        name(from.name),      
        sales_manager_id(from.sales_manager_id),      
        sold_to_user(from.sold_to_user),      
        status(from.status),      
        version(from.version)      
      {      
      }
      
      Campaign& Campaign::operator=(const Campaign& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        account = from.account;      
        bill_to_user = from.bill_to_user;      
        budget = from.budget;      
        budget_manual = from.budget_manual;      
        campaign_type = from.campaign_type;      
        commission = from.commission;      
        daily_budget = from.daily_budget;      
        date_end = from.date_end;      
        date_start = from.date_start;      
        delivery_pacing = from.delivery_pacing;      
        display_status_id = from.display_status_id;      
        flags = from.flags;      
        freq_cap = from.freq_cap;      
        last_deactivated = from.last_deactivated;      
        last_updated = from.last_updated;      
        marketplace = from.marketplace;      
        max_pub_share = from.max_pub_share;      
        name = from.name;      
        sales_manager_id = from.sales_manager_id;      
        sold_to_user = from.sold_to_user;      
        status = from.status;      
        version = from.version;      
        return *this;      
      }
      
      bool Campaign::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE campaign SET VERSION = now()";      
        {      
          strm << " WHERE campaign_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Campaign::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "bill_to_user_id,"      
            "budget,"      
            "budget_manual,"      
            "campaign_type,"      
            "commission,"      
            "daily_budget,"      
            "date_end,"      
            "date_start,"      
            "delivery_pacing,"      
            "display_status_id,"      
            "flags,"      
            "freq_cap_id,"      
            "last_deactivated,"      
            "last_updated,"      
            "marketplace,"      
            "max_pub_share,"      
            "name,"      
            "sales_manager_id,"      
            "sold_to_user_id,"      
            "status,"      
            "version "      
          "FROM campaign "      
          "WHERE campaign_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 22; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Campaign::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE campaign SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 22; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE campaign_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 22; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Campaign::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('campaign_campaign_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO campaign (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 22; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " campaign_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 22; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 22; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Campaign::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM campaign WHERE campaign_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Campaign::del ()      
      {      
        int status_id = get_display_status_id(conn, "campaign", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE campaign SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE campaign_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Campaign::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "campaign", status);      
        DB::Query  query(conn.get_query("UPDATE campaign SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE campaign_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Campaign::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Campaign" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  account = " << strof(account) << ";" << std::endl;      
          out << "  bill_to_user = " << strof(bill_to_user) << ";" << std::endl;      
          out << "  budget = " << strof(budget) << ";" << std::endl;      
          out << "  budget_manual = " << strof(budget_manual) << ";" << std::endl;      
          out << "  campaign_type = " << strof(campaign_type) << ";" << std::endl;      
          out << "  commission = " << strof(commission) << ";" << std::endl;      
          out << "  daily_budget = " << strof(daily_budget) << ";" << std::endl;      
          out << "  date_end = " << strof(date_end) << ";" << std::endl;      
          out << "  date_start = " << strof(date_start) << ";" << std::endl;      
          out << "  delivery_pacing = " << strof(delivery_pacing) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  freq_cap = " << strof(freq_cap) << ";" << std::endl;      
          out << "  last_deactivated = " << strof(last_deactivated) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  marketplace = " << strof(marketplace) << ";" << std::endl;      
          out << "  max_pub_share = " << strof(max_pub_share) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  sales_manager_id = " << strof(sales_manager_id) << ";" << std::endl;      
          out << "  sold_to_user = " << strof(sold_to_user) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Campaign because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Campaign& val)      
      {      
        out << "Campaign:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  account: " << val.account << std::endl;      
        out << "  bill_to_user: " << val.bill_to_user << std::endl;      
        out << "  budget: " << val.budget << std::endl;      
        out << "  budget_manual: " << val.budget_manual << std::endl;      
        out << "  campaign_type: " << val.campaign_type << std::endl;      
        out << "  commission: " << val.commission << std::endl;      
        out << "  daily_budget: " << val.daily_budget << std::endl;      
        out << "  date_end: " << val.date_end << std::endl;      
        out << "  date_start: " << val.date_start << std::endl;      
        out << "  delivery_pacing: " << val.delivery_pacing << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  freq_cap: " << val.freq_cap << std::endl;      
        out << "  last_deactivated: " << val.last_deactivated << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  marketplace: " << val.marketplace << std::endl;      
        out << "  max_pub_share: " << val.max_pub_share << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  sales_manager_id: " << val.sales_manager_id << std::endl;      
        out << "  sold_to_user: " << val.sold_to_user << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember CampaignCreative::members_[11] = {      
        { "ccg_id", &(null<CampaignCreative>()->ccg), 0},        
        { "creative_id", &(null<CampaignCreative>()->creative), 0},        
        { "display_status_id", &(null<CampaignCreative>()->display_status_id), "1"},        
        { "freq_cap_id", &(null<CampaignCreative>()->freq_cap), 0},        
        { "last_deactivated", &(null<CampaignCreative>()->last_deactivated), 0},        
        { "last_updated", &(null<CampaignCreative>()->last_updated), 0},        
        { "set_number", &(null<CampaignCreative>()->set_number), "1"},        
        { "status", &(null<CampaignCreative>()->status), "'A'"},        
        { "version", &(null<CampaignCreative>()->version), 0},        
        { "weight", &(null<CampaignCreative>()->weight), 0},        
        { "cc_id", &(null<CampaignCreative>()->id_), 0},        
      };
      
      CampaignCreative::CampaignCreative (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CampaignCreative::CampaignCreative (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CampaignCreative::CampaignCreative(const CampaignCreative& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        ccg(from.ccg),      
        creative(from.creative),      
        display_status_id(from.display_status_id),      
        freq_cap(from.freq_cap),      
        last_deactivated(from.last_deactivated),      
        last_updated(from.last_updated),      
        set_number(from.set_number),      
        status(from.status),      
        version(from.version),      
        weight(from.weight)      
      {      
      }
      
      CampaignCreative& CampaignCreative::operator=(const CampaignCreative& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        ccg = from.ccg;      
        creative = from.creative;      
        display_status_id = from.display_status_id;      
        freq_cap = from.freq_cap;      
        last_deactivated = from.last_deactivated;      
        last_updated = from.last_updated;      
        set_number = from.set_number;      
        status = from.status;      
        version = from.version;      
        weight = from.weight;      
        return *this;      
      }
      
      bool CampaignCreative::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE campaigncreative SET VERSION = now()";      
        {      
          strm << " WHERE cc_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreative::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "ccg_id,"      
            "creative_id,"      
            "display_status_id,"      
            "freq_cap_id,"      
            "last_deactivated,"      
            "last_updated,"      
            "set_number,"      
            "status,"      
            "version,"      
            "weight "      
          "FROM campaigncreative "      
          "WHERE cc_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 10; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CampaignCreative::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE campaigncreative SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 10; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE cc_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 10; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreative::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('campaigncreative_cc_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO campaigncreative (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 10; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " cc_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 10; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 10; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreative::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM campaigncreative WHERE cc_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CampaignCreative::del ()      
      {      
        return delet();      
      }
      
      bool CampaignCreative::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "campaigncreative", status);      
        DB::Query  query(conn.get_query("UPDATE campaigncreative SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE cc_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void CampaignCreative::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CampaignCreative" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  ccg = " << strof(ccg) << ";" << std::endl;      
          out << "  creative = " << strof(creative) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  freq_cap = " << strof(freq_cap) << ";" << std::endl;      
          out << "  last_deactivated = " << strof(last_deactivated) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  set_number = " << strof(set_number) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CampaignCreative because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CampaignCreative& val)      
      {      
        out << "CampaignCreative:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  ccg: " << val.ccg << std::endl;      
        out << "  creative: " << val.creative << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  freq_cap: " << val.freq_cap << std::endl;      
        out << "  last_deactivated: " << val.last_deactivated << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  set_number: " << val.set_number << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  weight: " << val.weight << std::endl;      
        return out;      
      }      
      ORMObjectMember CampaignCreativeGroup::members_[43] = {      
        { "budget", &(null<CampaignCreativeGroup>()->budget), "1000000"},        
        { "campaign_id", &(null<CampaignCreativeGroup>()->campaign), 0},        
        { "ccg_rate_id", &(null<CampaignCreativeGroup>()->ccg_rate), 0},        
        { "ccg_type", &(null<CampaignCreativeGroup>()->ccg_type), "'D'"},        
        { "channel_id", &(null<CampaignCreativeGroup>()->channel), 0},        
        { "channel_target", &(null<CampaignCreativeGroup>()->channel_target), "'A'"},        
        { "check_interval_num", &(null<CampaignCreativeGroup>()->check_interval_num), 0},        
        { "check_notes", &(null<CampaignCreativeGroup>()->check_notes), 0},        
        { "check_user_id", &(null<CampaignCreativeGroup>()->check_user_id), 0},        
        { "country_code", &(null<CampaignCreativeGroup>()->country_code), "'GN'"},        
        { "ctr_reset_id", &(null<CampaignCreativeGroup>()->ctr_reset_id), "0"},        
        { "cur_date", &(null<CampaignCreativeGroup>()->cur_date), 0},        
        { "daily_budget", &(null<CampaignCreativeGroup>()->daily_budget), 0},        
        { "daily_clicks", &(null<CampaignCreativeGroup>()->daily_clicks), 0},        
        { "daily_imp", &(null<CampaignCreativeGroup>()->daily_imp), 0},        
        { "date_end", &(null<CampaignCreativeGroup>()->date_end), 0},        
        { "date_start", &(null<CampaignCreativeGroup>()->date_start), "current_date - 1"},        
        { "delivery_pacing", &(null<CampaignCreativeGroup>()->delivery_pacing), "'D'"},        
        { "display_status_id", &(null<CampaignCreativeGroup>()->display_status_id), "1"},        
        { "flags", &(null<CampaignCreativeGroup>()->flags), "0"},        
        { "freq_cap_id", &(null<CampaignCreativeGroup>()->freq_cap), 0},        
        { "last_check_date", &(null<CampaignCreativeGroup>()->last_check_date), 0},        
        { "last_deactivated", &(null<CampaignCreativeGroup>()->last_deactivated), 0},        
        { "last_updated", &(null<CampaignCreativeGroup>()->last_updated), 0},        
        { "min_uid_age", &(null<CampaignCreativeGroup>()->min_uid_age), "0"},        
        { "name", &(null<CampaignCreativeGroup>()->name), 0},        
        { "next_check_date", &(null<CampaignCreativeGroup>()->next_check_date), 0},        
        { "optin_status_targeting", &(null<CampaignCreativeGroup>()->optin_status_targeting), "'YYY'"},        
        { "qa_date", &(null<CampaignCreativeGroup>()->qa_date), 0},        
        { "qa_description", &(null<CampaignCreativeGroup>()->qa_description), 0},        
        { "qa_status", &(null<CampaignCreativeGroup>()->qa_status), 0},        
        { "qa_user_id", &(null<CampaignCreativeGroup>()->qa_user_id), 0},        
        { "realized_budget", &(null<CampaignCreativeGroup>()->realized_budget), 0},        
        { "rotation_criteria", &(null<CampaignCreativeGroup>()->rotation_criteria), 0},        
        { "selected_mobile_operators", &(null<CampaignCreativeGroup>()->selected_mobile_operators), 0},        
        { "status", &(null<CampaignCreativeGroup>()->status), "'A'"},        
        { "targeting_channel_id", &(null<CampaignCreativeGroup>()->targeting_channel_id), 0},        
        { "tgt_type", &(null<CampaignCreativeGroup>()->tgt_type), "'C'"},        
        { "total_reach", &(null<CampaignCreativeGroup>()->total_reach), 0},        
        { "user_sample_group_end", &(null<CampaignCreativeGroup>()->user_sample_group_end), 0},        
        { "user_sample_group_start", &(null<CampaignCreativeGroup>()->user_sample_group_start), 0},        
        { "version", &(null<CampaignCreativeGroup>()->version), 0},        
        { "ccg_id", &(null<CampaignCreativeGroup>()->id_), 0},        
      };
      
      CampaignCreativeGroup::CampaignCreativeGroup (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CampaignCreativeGroup::CampaignCreativeGroup (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CampaignCreativeGroup::CampaignCreativeGroup(const CampaignCreativeGroup& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        budget(from.budget),      
        campaign(from.campaign),      
        ccg_rate(from.ccg_rate),      
        ccg_type(from.ccg_type),      
        channel(from.channel),      
        channel_target(from.channel_target),      
        check_interval_num(from.check_interval_num),      
        check_notes(from.check_notes),      
        check_user_id(from.check_user_id),      
        country_code(from.country_code),      
        ctr_reset_id(from.ctr_reset_id),      
        cur_date(from.cur_date),      
        daily_budget(from.daily_budget),      
        daily_clicks(from.daily_clicks),      
        daily_imp(from.daily_imp),      
        date_end(from.date_end),      
        date_start(from.date_start),      
        delivery_pacing(from.delivery_pacing),      
        display_status_id(from.display_status_id),      
        flags(from.flags),      
        freq_cap(from.freq_cap),      
        last_check_date(from.last_check_date),      
        last_deactivated(from.last_deactivated),      
        last_updated(from.last_updated),      
        min_uid_age(from.min_uid_age),      
        name(from.name),      
        next_check_date(from.next_check_date),      
        optin_status_targeting(from.optin_status_targeting),      
        qa_date(from.qa_date),      
        qa_description(from.qa_description),      
        qa_status(from.qa_status),      
        qa_user_id(from.qa_user_id),      
        realized_budget(from.realized_budget),      
        rotation_criteria(from.rotation_criteria),      
        selected_mobile_operators(from.selected_mobile_operators),      
        status(from.status),      
        targeting_channel_id(from.targeting_channel_id),      
        tgt_type(from.tgt_type),      
        total_reach(from.total_reach),      
        user_sample_group_end(from.user_sample_group_end),      
        user_sample_group_start(from.user_sample_group_start),      
        version(from.version)      
      {      
      }
      
      CampaignCreativeGroup& CampaignCreativeGroup::operator=(const CampaignCreativeGroup& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        budget = from.budget;      
        campaign = from.campaign;      
        ccg_rate = from.ccg_rate;      
        ccg_type = from.ccg_type;      
        channel = from.channel;      
        channel_target = from.channel_target;      
        check_interval_num = from.check_interval_num;      
        check_notes = from.check_notes;      
        check_user_id = from.check_user_id;      
        country_code = from.country_code;      
        ctr_reset_id = from.ctr_reset_id;      
        cur_date = from.cur_date;      
        daily_budget = from.daily_budget;      
        daily_clicks = from.daily_clicks;      
        daily_imp = from.daily_imp;      
        date_end = from.date_end;      
        date_start = from.date_start;      
        delivery_pacing = from.delivery_pacing;      
        display_status_id = from.display_status_id;      
        flags = from.flags;      
        freq_cap = from.freq_cap;      
        last_check_date = from.last_check_date;      
        last_deactivated = from.last_deactivated;      
        last_updated = from.last_updated;      
        min_uid_age = from.min_uid_age;      
        name = from.name;      
        next_check_date = from.next_check_date;      
        optin_status_targeting = from.optin_status_targeting;      
        qa_date = from.qa_date;      
        qa_description = from.qa_description;      
        qa_status = from.qa_status;      
        qa_user_id = from.qa_user_id;      
        realized_budget = from.realized_budget;      
        rotation_criteria = from.rotation_criteria;      
        selected_mobile_operators = from.selected_mobile_operators;      
        status = from.status;      
        targeting_channel_id = from.targeting_channel_id;      
        tgt_type = from.tgt_type;      
        total_reach = from.total_reach;      
        user_sample_group_end = from.user_sample_group_end;      
        user_sample_group_start = from.user_sample_group_start;      
        version = from.version;      
        return *this;      
      }
      
      bool CampaignCreativeGroup::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE campaigncreativegroup SET VERSION = now()";      
        {      
          strm << " WHERE ccg_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreativeGroup::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "budget,"      
            "campaign_id,"      
            "ccg_rate_id,"      
            "ccg_type,"      
            "channel_id,"      
            "channel_target,"      
            "check_interval_num,"      
            "check_notes,"      
            "check_user_id,"      
            "country_code,"      
            "ctr_reset_id,"      
            "cur_date,"      
            "daily_budget,"      
            "daily_clicks,"      
            "daily_imp,"      
            "date_end,"      
            "date_start,"      
            "delivery_pacing,"      
            "display_status_id,"      
            "flags,"      
            "freq_cap_id,"      
            "last_check_date,"      
            "last_deactivated,"      
            "last_updated,"      
            "min_uid_age,"      
            "name,"      
            "next_check_date,"      
            "optin_status_targeting,"      
            "qa_date,"      
            "qa_description,"      
            "qa_status,"      
            "qa_user_id,"      
            "realized_budget,"      
            "rotation_criteria,"      
            "selected_mobile_operators,"      
            "status,"      
            "targeting_channel_id,"      
            "tgt_type,"      
            "total_reach,"      
            "user_sample_group_end,"      
            "user_sample_group_start,"      
            "version "      
          "FROM campaigncreativegroup "      
          "WHERE ccg_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 42; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CampaignCreativeGroup::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE campaigncreativegroup SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 42; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE ccg_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 42; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreativeGroup::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('campaigncreativegroup_ccg_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO campaigncreativegroup (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 42; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " ccg_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 42; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 42; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CampaignCreativeGroup::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM campaigncreativegroup WHERE ccg_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CampaignCreativeGroup::del ()      
      {      
        int status_id = get_display_status_id(conn, "campaigncreativegroup", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE campaigncreativegroup SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE ccg_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool CampaignCreativeGroup::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "campaigncreativegroup", status);      
        DB::Query  query(conn.get_query("UPDATE campaigncreativegroup SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE ccg_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void CampaignCreativeGroup::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CampaignCreativeGroup" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  budget = " << strof(budget) << ";" << std::endl;      
          out << "  campaign = " << strof(campaign) << ";" << std::endl;      
          out << "  ccg_rate = " << strof(ccg_rate) << ";" << std::endl;      
          out << "  ccg_type = " << strof(ccg_type) << ";" << std::endl;      
          out << "  channel = " << strof(channel) << ";" << std::endl;      
          out << "  channel_target = " << strof(channel_target) << ";" << std::endl;      
          out << "  check_interval_num = " << strof(check_interval_num) << ";" << std::endl;      
          out << "  check_notes = " << strof(check_notes) << ";" << std::endl;      
          out << "  check_user_id = " << strof(check_user_id) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  ctr_reset_id = " << strof(ctr_reset_id) << ";" << std::endl;      
          out << "  cur_date = " << strof(cur_date) << ";" << std::endl;      
          out << "  daily_budget = " << strof(daily_budget) << ";" << std::endl;      
          out << "  daily_clicks = " << strof(daily_clicks) << ";" << std::endl;      
          out << "  daily_imp = " << strof(daily_imp) << ";" << std::endl;      
          out << "  date_end = " << strof(date_end) << ";" << std::endl;      
          out << "  date_start = " << strof(date_start) << ";" << std::endl;      
          out << "  delivery_pacing = " << strof(delivery_pacing) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  freq_cap = " << strof(freq_cap) << ";" << std::endl;      
          out << "  last_check_date = " << strof(last_check_date) << ";" << std::endl;      
          out << "  last_deactivated = " << strof(last_deactivated) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  min_uid_age = " << strof(min_uid_age) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  next_check_date = " << strof(next_check_date) << ";" << std::endl;      
          out << "  optin_status_targeting = " << strof(optin_status_targeting) << ";" << std::endl;      
          out << "  qa_date = " << strof(qa_date) << ";" << std::endl;      
          out << "  qa_description = " << strof(qa_description) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  qa_user_id = " << strof(qa_user_id) << ";" << std::endl;      
          out << "  realized_budget = " << strof(realized_budget) << ";" << std::endl;      
          out << "  rotation_criteria = " << strof(rotation_criteria) << ";" << std::endl;      
          out << "  selected_mobile_operators = " << strof(selected_mobile_operators) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  targeting_channel_id = " << strof(targeting_channel_id) << ";" << std::endl;      
          out << "  tgt_type = " << strof(tgt_type) << ";" << std::endl;      
          out << "  total_reach = " << strof(total_reach) << ";" << std::endl;      
          out << "  user_sample_group_end = " << strof(user_sample_group_end) << ";" << std::endl;      
          out << "  user_sample_group_start = " << strof(user_sample_group_start) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CampaignCreativeGroup because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CampaignCreativeGroup& val)      
      {      
        out << "CampaignCreativeGroup:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  budget: " << val.budget << std::endl;      
        out << "  campaign: " << val.campaign << std::endl;      
        out << "  ccg_rate: " << val.ccg_rate << std::endl;      
        out << "  ccg_type: " << val.ccg_type << std::endl;      
        out << "  channel: " << val.channel << std::endl;      
        out << "  channel_target: " << val.channel_target << std::endl;      
        out << "  check_interval_num: " << val.check_interval_num << std::endl;      
        out << "  check_notes: " << val.check_notes << std::endl;      
        out << "  check_user_id: " << val.check_user_id << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  ctr_reset_id: " << val.ctr_reset_id << std::endl;      
        out << "  cur_date: " << val.cur_date << std::endl;      
        out << "  daily_budget: " << val.daily_budget << std::endl;      
        out << "  daily_clicks: " << val.daily_clicks << std::endl;      
        out << "  daily_imp: " << val.daily_imp << std::endl;      
        out << "  date_end: " << val.date_end << std::endl;      
        out << "  date_start: " << val.date_start << std::endl;      
        out << "  delivery_pacing: " << val.delivery_pacing << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  freq_cap: " << val.freq_cap << std::endl;      
        out << "  last_check_date: " << val.last_check_date << std::endl;      
        out << "  last_deactivated: " << val.last_deactivated << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  min_uid_age: " << val.min_uid_age << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  next_check_date: " << val.next_check_date << std::endl;      
        out << "  optin_status_targeting: " << val.optin_status_targeting << std::endl;      
        out << "  qa_date: " << val.qa_date << std::endl;      
        out << "  qa_description: " << val.qa_description << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  qa_user_id: " << val.qa_user_id << std::endl;      
        out << "  realized_budget: " << val.realized_budget << std::endl;      
        out << "  rotation_criteria: " << val.rotation_criteria << std::endl;      
        out << "  selected_mobile_operators: " << val.selected_mobile_operators << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  targeting_channel_id: " << val.targeting_channel_id << std::endl;      
        out << "  tgt_type: " << val.tgt_type << std::endl;      
        out << "  total_reach: " << val.total_reach << std::endl;      
        out << "  user_sample_group_end: " << val.user_sample_group_end << std::endl;      
        out << "  user_sample_group_start: " << val.user_sample_group_start << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Ccgaction::members_[3] = {      
        { "last_updated", &(null<Ccgaction>()->last_updated), 0},        
        { "action_id", &(null<Ccgaction>()->action_id_), 0},        
        { "ccg_id", &(null<Ccgaction>()->ccg_id_), 0},        
      };
      
      Ccgaction::Ccgaction (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Ccgaction::Ccgaction (DB::IConn& connection,      
        const ORMInt::value_type& action_id,        
        const ORMInt::value_type& ccg_id        
      )      
        :ORMObject<postgres_connection>(connection), action_id_(action_id), ccg_id_(ccg_id)
      {}
      
      Ccgaction::Ccgaction(const Ccgaction& from)      
        :ORMObject<postgres_connection>(from), action_id_(from.action_id_), ccg_id_(from.ccg_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      Ccgaction& Ccgaction::operator=(const Ccgaction& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool Ccgaction::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM ccgaction "      
          "WHERE action_id = :i1 AND ccg_id = :i2"));      
        query  << action_id_ << ccg_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Ccgaction::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE ccgaction SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE action_id = :i1 AND ccg_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << action_id_ << ccg_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Ccgaction::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (action_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(action_id) + 1 FROM ccgaction"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> action_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO ccgaction (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " action_id, ccg_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << action_id_ << ccg_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Ccgaction::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM ccgaction WHERE action_id = :i1 AND ccg_id = :i2"));      
        query  << action_id_ << ccg_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Ccgaction::del ()      
      {      
        return delet();      
      }
      
      void Ccgaction::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Ccgaction" << std::endl;      
          out << "{" << std::endl;      
          out << "* action_id = " << strof(action_id_) << ";" << std::endl;      
          out << "* ccg_id = " << strof(ccg_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Ccgaction because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Ccgaction& val)      
      {      
        out << "Ccgaction:" << std::endl;      
        out << " *action_id: " << val.action_id_ << std::endl;      
        out << " *ccg_id: " << val.ccg_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember CCGKeyword::members_[10] = {      
        { "ccg_id", &(null<CCGKeyword>()->ccg), 0},        
        { "channel_id", &(null<CCGKeyword>()->channel_id), 0},        
        { "click_url", &(null<CCGKeyword>()->click_url), 0},        
        { "last_updated", &(null<CCGKeyword>()->last_updated), 0},        
        { "max_cpc_bid", &(null<CCGKeyword>()->max_cpc_bid), 0},        
        { "original_keyword", &(null<CCGKeyword>()->original_keyword), 0},        
        { "status", &(null<CCGKeyword>()->status), 0},        
        { "trigger_type", &(null<CCGKeyword>()->trigger_type), 0},        
        { "version", &(null<CCGKeyword>()->version), 0},        
        { "ccg_keyword_id", &(null<CCGKeyword>()->id_), 0},        
      };
      
      CCGKeyword::CCGKeyword (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CCGKeyword::CCGKeyword (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CCGKeyword::CCGKeyword(const CCGKeyword& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        ccg(from.ccg),      
        channel_id(from.channel_id),      
        click_url(from.click_url),      
        last_updated(from.last_updated),      
        max_cpc_bid(from.max_cpc_bid),      
        original_keyword(from.original_keyword),      
        status(from.status),      
        trigger_type(from.trigger_type),      
        version(from.version)      
      {      
      }
      
      CCGKeyword& CCGKeyword::operator=(const CCGKeyword& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        ccg = from.ccg;      
        channel_id = from.channel_id;      
        click_url = from.click_url;      
        last_updated = from.last_updated;      
        max_cpc_bid = from.max_cpc_bid;      
        original_keyword = from.original_keyword;      
        status = from.status;      
        trigger_type = from.trigger_type;      
        version = from.version;      
        return *this;      
      }
      
      bool CCGKeyword::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE ccgkeyword SET VERSION = now()";      
        {      
          strm << " WHERE ccg_keyword_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGKeyword::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "ccg_id,"      
            "channel_id,"      
            "click_url,"      
            "last_updated,"      
            "max_cpc_bid,"      
            "original_keyword,"      
            "status,"      
            "trigger_type,"      
            "version "      
          "FROM ccgkeyword "      
          "WHERE ccg_keyword_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CCGKeyword::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE ccgkeyword SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE ccg_keyword_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGKeyword::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('ccgkeyword_ccg_keyword_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO ccgkeyword (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " ccg_keyword_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGKeyword::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM ccgkeyword WHERE ccg_keyword_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CCGKeyword::del ()      
      {      
        return delet();      
      }
      
      void CCGKeyword::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CCGKeyword" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  ccg = " << strof(ccg) << ";" << std::endl;      
          out << "  channel_id = " << strof(channel_id) << ";" << std::endl;      
          out << "  click_url = " << strof(click_url) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  max_cpc_bid = " << strof(max_cpc_bid) << ";" << std::endl;      
          out << "  original_keyword = " << strof(original_keyword) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  trigger_type = " << strof(trigger_type) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CCGKeyword because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CCGKeyword& val)      
      {      
        out << "CCGKeyword:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  ccg: " << val.ccg << std::endl;      
        out << "  channel_id: " << val.channel_id << std::endl;      
        out << "  click_url: " << val.click_url << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  max_cpc_bid: " << val.max_cpc_bid << std::endl;      
        out << "  original_keyword: " << val.original_keyword << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  trigger_type: " << val.trigger_type << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember CCGRate::members_[8] = {      
        { "ccg_id", &(null<CCGRate>()->ccg), 0},        
        { "cpa", &(null<CCGRate>()->cpa), 0},        
        { "cpc", &(null<CCGRate>()->cpc), 0},        
        { "cpm", &(null<CCGRate>()->cpm), 0},        
        { "effective_date", &(null<CCGRate>()->effective_date), 0},        
        { "last_updated", &(null<CCGRate>()->last_updated), 0},        
        { "rate_type", &(null<CCGRate>()->rate_type), 0},        
        { "ccg_rate_id", &(null<CCGRate>()->id_), 0},        
      };
      
      CCGRate::CCGRate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CCGRate::CCGRate (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CCGRate::CCGRate(const CCGRate& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        ccg(from.ccg),      
        cpa(from.cpa),      
        cpc(from.cpc),      
        cpm(from.cpm),      
        effective_date(from.effective_date),      
        last_updated(from.last_updated),      
        rate_type(from.rate_type)      
      {      
      }
      
      CCGRate& CCGRate::operator=(const CCGRate& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        ccg = from.ccg;      
        cpa = from.cpa;      
        cpc = from.cpc;      
        cpm = from.cpm;      
        effective_date = from.effective_date;      
        last_updated = from.last_updated;      
        rate_type = from.rate_type;      
        return *this;      
      }
      
      bool CCGRate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "ccg_id,"      
            "cpa,"      
            "cpc,"      
            "cpm,"      
            "effective_date,"      
            "last_updated,"      
            "rate_type "      
          "FROM ccgrate "      
          "WHERE ccg_rate_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CCGRate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE ccgrate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE ccg_rate_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGRate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('ccgrate_ccg_rate_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO ccgrate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " ccg_rate_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGRate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM ccgrate WHERE ccg_rate_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CCGRate::del ()      
      {      
        return delet();      
      }
      
      void CCGRate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CCGRate" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  ccg = " << strof(ccg) << ";" << std::endl;      
          out << "  cpa = " << strof(cpa) << ";" << std::endl;      
          out << "  cpc = " << strof(cpc) << ";" << std::endl;      
          out << "  cpm = " << strof(cpm) << ";" << std::endl;      
          out << "  effective_date = " << strof(effective_date) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CCGRate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CCGRate& val)      
      {      
        out << "CCGRate:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  ccg: " << val.ccg << std::endl;      
        out << "  cpa: " << val.cpa << std::endl;      
        out << "  cpc: " << val.cpc << std::endl;      
        out << "  cpm: " << val.cpm << std::endl;      
        out << "  effective_date: " << val.effective_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  rate_type: " << val.rate_type << std::endl;      
        return out;      
      }      
      ORMObjectMember CCGSite::members_[3] = {      
        { "last_updated", &(null<CCGSite>()->last_updated), 0},        
        { "ccg_id", &(null<CCGSite>()->ccg_id_), 0},        
        { "site_id", &(null<CCGSite>()->site_id_), 0},        
      };
      
      CCGSite::CCGSite (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CCGSite::CCGSite (DB::IConn& connection,      
        const ORMInt::value_type& ccg_id,        
        const ORMInt::value_type& site_id        
      )      
        :ORMObject<postgres_connection>(connection), ccg_id_(ccg_id), site_id_(site_id)
      {}
      
      CCGSite::CCGSite(const CCGSite& from)      
        :ORMObject<postgres_connection>(from), ccg_id_(from.ccg_id_), site_id_(from.site_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      CCGSite& CCGSite::operator=(const CCGSite& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool CCGSite::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM ccgsite "      
          "WHERE ccg_id = :i1 AND site_id = :i2"));      
        query  << ccg_id_ << site_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CCGSite::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE ccgsite SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE ccg_id = :i1 AND site_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << ccg_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGSite::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (ccg_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(ccg_id) + 1 FROM ccgsite"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> ccg_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO ccgsite (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " ccg_id, site_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << ccg_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CCGSite::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM ccgsite WHERE ccg_id = :i1 AND site_id = :i2"));      
        query  << ccg_id_ << site_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CCGSite::del ()      
      {      
        return delet();      
      }
      
      void CCGSite::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CCGSite" << std::endl;      
          out << "{" << std::endl;      
          out << "* ccg_id = " << strof(ccg_id_) << ";" << std::endl;      
          out << "* site_id = " << strof(site_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CCGSite because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CCGSite& val)      
      {      
        out << "CCGSite:" << std::endl;      
        out << " *ccg_id: " << val.ccg_id_ << std::endl;      
        out << " *site_id: " << val.site_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember Channel::members_[51] = {      
        { "account_id", &(null<Channel>()->account), 0},        
        { "address", &(null<Channel>()->address), 0},        
        { "base_keyword", &(null<Channel>()->base_keyword), 0},        
        { "behav_params_list_id", &(null<Channel>()->behav_params_list_id), 0},        
        { "channel_list_id", &(null<Channel>()->channel_list_id), 0},        
        { "channel_name_macro", &(null<Channel>()->channel_name_macro), 0},        
        { "channel_rate_id", &(null<Channel>()->channel_rate_id), 0},        
        { "channel_type", &(null<Channel>()->type), 0},        
        { "check_interval_num", &(null<Channel>()->check_interval_num), 0},        
        { "check_notes", &(null<Channel>()->check_notes), 0},        
        { "check_user_id", &(null<Channel>()->check_user_id), 0},        
        { "city_list", &(null<Channel>()->city_list), 0},        
        { "country_code", &(null<Channel>()->country_code), "'GN'"},        
        { "created_date", &(null<Channel>()->created_date), 0},        
        { "description", &(null<Channel>()->description), 0},        
        { "discover_annotation", &(null<Channel>()->discover_annotation), 0},        
        { "discover_query", &(null<Channel>()->discover_query), 0},        
        { "display_status_id", &(null<Channel>()->display_status_id), "1"},        
        { "distinct_url_triggers_count", &(null<Channel>()->distinct_url_triggers_count), 0},        
        { "expression", &(null<Channel>()->expression), 0},        
        { "flags", &(null<Channel>()->flags), 0},        
        { "freq_cap_id", &(null<Channel>()->freq_cap_id), 0},        
        { "geo_type", &(null<Channel>()->geo_type), 0},        
        { "keyword_trigger_macro", &(null<Channel>()->keyword_trigger_macro), 0},        
        { "language", &(null<Channel>()->language), 0},        
        { "last_check_date", &(null<Channel>()->last_check_date), 0},        
        { "last_updated", &(null<Channel>()->last_updated), 0},        
        { "latitude", &(null<Channel>()->latitude), 0},        
        { "longitude", &(null<Channel>()->longitude), 0},        
        { "message_sent", &(null<Channel>()->message_sent), "0"},        
        { "name", &(null<Channel>()->name), 0},        
        { "namespace", &(null<Channel>()->channel_namespace), 0},        
        { "newsgate_category_name", &(null<Channel>()->newsgate_category_name), 0},        
        { "next_check_date", &(null<Channel>()->next_check_date), 0},        
        { "parent_channel_id", &(null<Channel>()->parent_channel_id), 0},        
        { "qa_date", &(null<Channel>()->qa_date), 0},        
        { "qa_description", &(null<Channel>()->qa_description), 0},        
        { "qa_status", &(null<Channel>()->qa_status), 0},        
        { "qa_user_id", &(null<Channel>()->qa_user_id), 0},        
        { "radius", &(null<Channel>()->radius), 0},        
        { "radius_units", &(null<Channel>()->radius_units), 0},        
        { "size_id", &(null<Channel>()->size_id), 0},        
        { "status", &(null<Channel>()->status), 0},        
        { "status_change_date", &(null<Channel>()->status_change_date), 0},        
        { "superseded_by_channel_id", &(null<Channel>()->superseded_by_channel_id), 0},        
        { "trigger_type", &(null<Channel>()->trigger_type), 0},        
        { "triggers_status", &(null<Channel>()->triggers_status), 0},        
        { "triggers_version", &(null<Channel>()->triggers_version), 0},        
        { "version", &(null<Channel>()->version), 0},        
        { "visibility", &(null<Channel>()->visibility), "'PUB'"},        
        { "channel_id", &(null<Channel>()->id_), 0},        
      };
      
      Channel::Channel (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Channel::Channel (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Channel::Channel(const Channel& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        account(from.account),      
        address(from.address),      
        base_keyword(from.base_keyword),      
        behav_params_list_id(from.behav_params_list_id),      
        channel_list_id(from.channel_list_id),      
        channel_name_macro(from.channel_name_macro),      
        channel_rate_id(from.channel_rate_id),      
        type(from.type),      
        check_interval_num(from.check_interval_num),      
        check_notes(from.check_notes),      
        check_user_id(from.check_user_id),      
        city_list(from.city_list),      
        country_code(from.country_code),      
        created_date(from.created_date),      
        description(from.description),      
        discover_annotation(from.discover_annotation),      
        discover_query(from.discover_query),      
        display_status_id(from.display_status_id),      
        distinct_url_triggers_count(from.distinct_url_triggers_count),      
        expression(from.expression),      
        flags(from.flags),      
        freq_cap_id(from.freq_cap_id),      
        geo_type(from.geo_type),      
        keyword_trigger_macro(from.keyword_trigger_macro),      
        language(from.language),      
        last_check_date(from.last_check_date),      
        last_updated(from.last_updated),      
        latitude(from.latitude),      
        longitude(from.longitude),      
        message_sent(from.message_sent),      
        name(from.name),      
        channel_namespace(from.channel_namespace),      
        newsgate_category_name(from.newsgate_category_name),      
        next_check_date(from.next_check_date),      
        parent_channel_id(from.parent_channel_id),      
        qa_date(from.qa_date),      
        qa_description(from.qa_description),      
        qa_status(from.qa_status),      
        qa_user_id(from.qa_user_id),      
        radius(from.radius),      
        radius_units(from.radius_units),      
        size_id(from.size_id),      
        status(from.status),      
        status_change_date(from.status_change_date),      
        superseded_by_channel_id(from.superseded_by_channel_id),      
        trigger_type(from.trigger_type),      
        triggers_status(from.triggers_status),      
        triggers_version(from.triggers_version),      
        version(from.version),      
        visibility(from.visibility)      
      {      
      }
      
      Channel& Channel::operator=(const Channel& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        account = from.account;      
        address = from.address;      
        base_keyword = from.base_keyword;      
        behav_params_list_id = from.behav_params_list_id;      
        channel_list_id = from.channel_list_id;      
        channel_name_macro = from.channel_name_macro;      
        channel_rate_id = from.channel_rate_id;      
        type = from.type;      
        check_interval_num = from.check_interval_num;      
        check_notes = from.check_notes;      
        check_user_id = from.check_user_id;      
        city_list = from.city_list;      
        country_code = from.country_code;      
        created_date = from.created_date;      
        description = from.description;      
        discover_annotation = from.discover_annotation;      
        discover_query = from.discover_query;      
        display_status_id = from.display_status_id;      
        distinct_url_triggers_count = from.distinct_url_triggers_count;      
        expression = from.expression;      
        flags = from.flags;      
        freq_cap_id = from.freq_cap_id;      
        geo_type = from.geo_type;      
        keyword_trigger_macro = from.keyword_trigger_macro;      
        language = from.language;      
        last_check_date = from.last_check_date;      
        last_updated = from.last_updated;      
        latitude = from.latitude;      
        longitude = from.longitude;      
        message_sent = from.message_sent;      
        name = from.name;      
        channel_namespace = from.channel_namespace;      
        newsgate_category_name = from.newsgate_category_name;      
        next_check_date = from.next_check_date;      
        parent_channel_id = from.parent_channel_id;      
        qa_date = from.qa_date;      
        qa_description = from.qa_description;      
        qa_status = from.qa_status;      
        qa_user_id = from.qa_user_id;      
        radius = from.radius;      
        radius_units = from.radius_units;      
        size_id = from.size_id;      
        status = from.status;      
        status_change_date = from.status_change_date;      
        superseded_by_channel_id = from.superseded_by_channel_id;      
        trigger_type = from.trigger_type;      
        triggers_status = from.triggers_status;      
        triggers_version = from.triggers_version;      
        version = from.version;      
        visibility = from.visibility;      
        return *this;      
      }
      
      bool Channel::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE channel SET VERSION = now()";      
        {      
          strm << " WHERE channel_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channel::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "address,"      
            "base_keyword,"      
            "behav_params_list_id,"      
            "channel_list_id,"      
            "channel_name_macro,"      
            "channel_rate_id,"      
            "channel_type,"      
            "check_interval_num,"      
            "check_notes,"      
            "check_user_id,"      
            "city_list,"      
            "country_code,"      
            "created_date,"      
            "description,"      
            "discover_annotation,"      
            "discover_query,"      
            "display_status_id,"      
            "distinct_url_triggers_count,"      
            "expression,"      
            "flags,"      
            "freq_cap_id,"      
            "geo_type,"      
            "keyword_trigger_macro,"      
            "language,"      
            "last_check_date,"      
            "last_updated,"      
            "latitude,"      
            "longitude,"      
            "message_sent,"      
            "name,"      
            "namespace,"      
            "newsgate_category_name,"      
            "next_check_date,"      
            "parent_channel_id,"      
            "qa_date,"      
            "qa_description,"      
            "qa_status,"      
            "qa_user_id,"      
            "radius,"      
            "radius_units,"      
            "size_id,"      
            "status,"      
            "status_change_date,"      
            "superseded_by_channel_id,"      
            "trigger_type,"      
            "triggers_status,"      
            "triggers_version,"      
            "version,"      
            "visibility "      
          "FROM channel "      
          "WHERE channel_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 50; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Channel::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE channel SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 50; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE channel_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 50; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channel::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('channel_channel_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO channel (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 50; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " channel_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 50; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 50; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channel::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM channel WHERE channel_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Channel::del ()      
      {      
        int status_id = get_display_status_id(conn, "channel", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE channel SET"      
          "    DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE channel_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Channel::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "channel", status);      
        DB::Query  query(conn.get_query("UPDATE channel SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE channel_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Channel::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Channel" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  account = " << strof(account) << ";" << std::endl;      
          out << "  address = " << strof(address) << ";" << std::endl;      
          out << "  base_keyword = " << strof(base_keyword) << ";" << std::endl;      
          out << "  behav_params_list_id = " << strof(behav_params_list_id) << ";" << std::endl;      
          out << "  channel_list_id = " << strof(channel_list_id) << ";" << std::endl;      
          out << "  channel_name_macro = " << strof(channel_name_macro) << ";" << std::endl;      
          out << "  channel_rate_id = " << strof(channel_rate_id) << ";" << std::endl;      
          out << "  type = " << strof(type) << ";" << std::endl;      
          out << "  check_interval_num = " << strof(check_interval_num) << ";" << std::endl;      
          out << "  check_notes = " << strof(check_notes) << ";" << std::endl;      
          out << "  check_user_id = " << strof(check_user_id) << ";" << std::endl;      
          out << "  city_list = " << strof(city_list) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  created_date = " << strof(created_date) << ";" << std::endl;      
          out << "  description = " << strof(description) << ";" << std::endl;      
          out << "  discover_annotation = " << strof(discover_annotation) << ";" << std::endl;      
          out << "  discover_query = " << strof(discover_query) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  distinct_url_triggers_count = " << strof(distinct_url_triggers_count) << ";" << std::endl;      
          out << "  expression = " << strof(expression) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  freq_cap_id = " << strof(freq_cap_id) << ";" << std::endl;      
          out << "  geo_type = " << strof(geo_type) << ";" << std::endl;      
          out << "  keyword_trigger_macro = " << strof(keyword_trigger_macro) << ";" << std::endl;      
          out << "  language = " << strof(language) << ";" << std::endl;      
          out << "  last_check_date = " << strof(last_check_date) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  latitude = " << strof(latitude) << ";" << std::endl;      
          out << "  longitude = " << strof(longitude) << ";" << std::endl;      
          out << "  message_sent = " << strof(message_sent) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  channel_namespace = " << strof(channel_namespace) << ";" << std::endl;      
          out << "  newsgate_category_name = " << strof(newsgate_category_name) << ";" << std::endl;      
          out << "  next_check_date = " << strof(next_check_date) << ";" << std::endl;      
          out << "  parent_channel_id = " << strof(parent_channel_id) << ";" << std::endl;      
          out << "  qa_date = " << strof(qa_date) << ";" << std::endl;      
          out << "  qa_description = " << strof(qa_description) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  qa_user_id = " << strof(qa_user_id) << ";" << std::endl;      
          out << "  radius = " << strof(radius) << ";" << std::endl;      
          out << "  radius_units = " << strof(radius_units) << ";" << std::endl;      
          out << "  size_id = " << strof(size_id) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  status_change_date = " << strof(status_change_date) << ";" << std::endl;      
          out << "  superseded_by_channel_id = " << strof(superseded_by_channel_id) << ";" << std::endl;      
          out << "  trigger_type = " << strof(trigger_type) << ";" << std::endl;      
          out << "  triggers_status = " << strof(triggers_status) << ";" << std::endl;      
          out << "  triggers_version = " << strof(triggers_version) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Channel because not select");      
        }      
      }
      
      bool Channel::has_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query query(conn.get_query("SELECT "      
            "channel_id "      
          "FROM channel "      
          "WHERE name = :i1"));      
        query.set(name);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> this->id_;      
          return true;      
        }      
        return false;      
      }
      
      bool Channel::select_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "address,"      
            "base_keyword,"      
            "behav_params_list_id,"      
            "channel_list_id,"      
            "channel_name_macro,"      
            "channel_rate_id,"      
            "channel_type,"      
            "check_interval_num,"      
            "check_notes,"      
            "check_user_id,"      
            "city_list,"      
            "country_code,"      
            "created_date,"      
            "description,"      
            "discover_annotation,"      
            "discover_query,"      
            "display_status_id,"      
            "distinct_url_triggers_count,"      
            "expression,"      
            "flags,"      
            "freq_cap_id,"      
            "geo_type,"      
            "keyword_trigger_macro,"      
            "language,"      
            "last_check_date,"      
            "last_updated,"      
            "latitude,"      
            "longitude,"      
            "message_sent,"      
            "namespace,"      
            "newsgate_category_name,"      
            "next_check_date,"      
            "parent_channel_id,"      
            "qa_date,"      
            "qa_description,"      
            "qa_status,"      
            "qa_user_id,"      
            "radius,"      
            "radius_units,"      
            "size_id,"      
            "status,"      
            "status_change_date,"      
            "superseded_by_channel_id,"      
            "trigger_type,"      
            "triggers_status,"      
            "triggers_version,"      
            "version,"      
            "visibility,"      
            "channel_id "      
          "FROM channel "      
          "WHERE name = :i1"));      
        query.set(name);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> account      
                 >> address      
                 >> base_keyword      
                 >> behav_params_list_id      
                 >> channel_list_id      
                 >> channel_name_macro      
                 >> channel_rate_id      
                 >> type      
                 >> check_interval_num      
                 >> check_notes      
                 >> check_user_id      
                 >> city_list      
                 >> country_code      
                 >> created_date      
                 >> description      
                 >> discover_annotation      
                 >> discover_query      
                 >> display_status_id      
                 >> distinct_url_triggers_count      
                 >> expression      
                 >> flags      
                 >> freq_cap_id      
                 >> geo_type      
                 >> keyword_trigger_macro      
                 >> language      
                 >> last_check_date      
                 >> last_updated      
                 >> latitude      
                 >> longitude      
                 >> message_sent      
                 >> channel_namespace      
                 >> newsgate_category_name      
                 >> next_check_date      
                 >> parent_channel_id      
                 >> qa_date      
                 >> qa_description      
                 >> qa_status      
                 >> qa_user_id      
                 >> radius      
                 >> radius_units      
                 >> size_id      
                 >> status      
                 >> status_change_date      
                 >> superseded_by_channel_id      
                 >> trigger_type      
                 >> triggers_status      
                 >> triggers_version      
                 >> version      
                 >> visibility      
                 >> id_;      
          return true;      
        }      
        return false;      
      }
      
      bool Channel::update_name (const ORMString::value_type& name, bool set_defaults )      
      {      
        Unused(name);        
        Unused(set_defaults);        
        this->name = name;        
        std::ostringstream strm;      
        strm << "UPDATE channel SET ";      
        int counter = 1;      
          counter = update_(strm, counter, this, members_[0], set_defaults);      
          counter = update_(strm, counter, this, members_[1], set_defaults);      
          counter = update_(strm, counter, this, members_[2], set_defaults);      
          counter = update_(strm, counter, this, members_[3], set_defaults);      
          counter = update_(strm, counter, this, members_[4], set_defaults);      
          counter = update_(strm, counter, this, members_[5], set_defaults);      
          counter = update_(strm, counter, this, members_[6], set_defaults);      
          counter = update_(strm, counter, this, members_[7], set_defaults);      
          counter = update_(strm, counter, this, members_[8], set_defaults);      
          counter = update_(strm, counter, this, members_[9], set_defaults);      
          counter = update_(strm, counter, this, members_[10], set_defaults);      
          counter = update_(strm, counter, this, members_[11], set_defaults);      
          counter = update_(strm, counter, this, members_[12], set_defaults);      
          counter = update_(strm, counter, this, members_[13], set_defaults);      
          counter = update_(strm, counter, this, members_[14], set_defaults);      
          counter = update_(strm, counter, this, members_[15], set_defaults);      
          counter = update_(strm, counter, this, members_[16], set_defaults);      
          counter = update_(strm, counter, this, members_[17], set_defaults);      
          counter = update_(strm, counter, this, members_[18], set_defaults);      
          counter = update_(strm, counter, this, members_[19], set_defaults);      
          counter = update_(strm, counter, this, members_[20], set_defaults);      
          counter = update_(strm, counter, this, members_[21], set_defaults);      
          counter = update_(strm, counter, this, members_[22], set_defaults);      
          counter = update_(strm, counter, this, members_[23], set_defaults);      
          counter = update_(strm, counter, this, members_[24], set_defaults);      
          counter = update_(strm, counter, this, members_[25], set_defaults);      
          counter = update_(strm, counter, this, members_[26], set_defaults);      
          counter = update_(strm, counter, this, members_[27], set_defaults);      
          counter = update_(strm, counter, this, members_[28], set_defaults);      
          counter = update_(strm, counter, this, members_[29], set_defaults);      
          counter = update_(strm, counter, this, members_[31], set_defaults);      
          counter = update_(strm, counter, this, members_[32], set_defaults);      
          counter = update_(strm, counter, this, members_[33], set_defaults);      
          counter = update_(strm, counter, this, members_[34], set_defaults);      
          counter = update_(strm, counter, this, members_[35], set_defaults);      
          counter = update_(strm, counter, this, members_[36], set_defaults);      
          counter = update_(strm, counter, this, members_[37], set_defaults);      
          counter = update_(strm, counter, this, members_[38], set_defaults);      
          counter = update_(strm, counter, this, members_[39], set_defaults);      
          counter = update_(strm, counter, this, members_[40], set_defaults);      
          counter = update_(strm, counter, this, members_[41], set_defaults);      
          counter = update_(strm, counter, this, members_[42], set_defaults);      
          counter = update_(strm, counter, this, members_[43], set_defaults);      
          counter = update_(strm, counter, this, members_[44], set_defaults);      
          counter = update_(strm, counter, this, members_[45], set_defaults);      
          counter = update_(strm, counter, this, members_[46], set_defaults);      
          counter = update_(strm, counter, this, members_[47], set_defaults);      
          counter = update_(strm, counter, this, members_[48], set_defaults);      
          counter = update_(strm, counter, this, members_[49], set_defaults);      
        if(counter > 1)      
        {      
          strm << " WHERE name = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          setin_(query, account);      
          setin_(query, address);      
          setin_(query, base_keyword);      
          setin_(query, behav_params_list_id);      
          setin_(query, channel_list_id);      
          setin_(query, channel_name_macro);      
          setin_(query, channel_rate_id);      
          setin_(query, type);      
          setin_(query, check_interval_num);      
          setin_(query, check_notes);      
          setin_(query, check_user_id);      
          setin_(query, city_list);      
          setin_(query, country_code);      
          setin_(query, created_date);      
          setin_(query, description);      
          setin_(query, discover_annotation);      
          setin_(query, discover_query);      
          setin_(query, display_status_id);      
          setin_(query, distinct_url_triggers_count);      
          setin_(query, expression);      
          setin_(query, flags);      
          setin_(query, freq_cap_id);      
          setin_(query, geo_type);      
          setin_(query, keyword_trigger_macro);      
          setin_(query, language);      
          setin_(query, last_check_date);      
          setin_(query, last_updated);      
          setin_(query, latitude);      
          setin_(query, longitude);      
          setin_(query, message_sent);      
          setin_(query, channel_namespace);      
          setin_(query, newsgate_category_name);      
          setin_(query, next_check_date);      
          setin_(query, parent_channel_id);      
          setin_(query, qa_date);      
          setin_(query, qa_description);      
          setin_(query, qa_status);      
          setin_(query, qa_user_id);      
          setin_(query, radius);      
          setin_(query, radius_units);      
          setin_(query, size_id);      
          setin_(query, status);      
          setin_(query, status_change_date);      
          setin_(query, superseded_by_channel_id);      
          setin_(query, trigger_type);      
          setin_(query, triggers_status);      
          setin_(query, triggers_version);      
          setin_(query, version);      
          setin_(query, visibility);      
          query.set(name);      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channel::delet_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query  query(conn.get_query("DELETE FROM channel WHERE name = :i1"));      
        query.set(name);      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Channel::del_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query  query(conn.get_query("UPDATE channel SET"       
          "    NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE name = :i1"));      
        query.set(name);      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      std::ostream& operator<< (std::ostream& out, const Channel& val)      
      {      
        out << "Channel:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  account: " << val.account << std::endl;      
        out << "  address: " << val.address << std::endl;      
        out << "  base_keyword: " << val.base_keyword << std::endl;      
        out << "  behav_params_list_id: " << val.behav_params_list_id << std::endl;      
        out << "  channel_list_id: " << val.channel_list_id << std::endl;      
        out << "  channel_name_macro: " << val.channel_name_macro << std::endl;      
        out << "  channel_rate_id: " << val.channel_rate_id << std::endl;      
        out << "  type: " << val.type << std::endl;      
        out << "  check_interval_num: " << val.check_interval_num << std::endl;      
        out << "  check_notes: " << val.check_notes << std::endl;      
        out << "  check_user_id: " << val.check_user_id << std::endl;      
        out << "  city_list: " << val.city_list << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  created_date: " << val.created_date << std::endl;      
        out << "  description: " << val.description << std::endl;      
        out << "  discover_annotation: " << val.discover_annotation << std::endl;      
        out << "  discover_query: " << val.discover_query << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  distinct_url_triggers_count: " << val.distinct_url_triggers_count << std::endl;      
        out << "  expression: " << val.expression << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  freq_cap_id: " << val.freq_cap_id << std::endl;      
        out << "  geo_type: " << val.geo_type << std::endl;      
        out << "  keyword_trigger_macro: " << val.keyword_trigger_macro << std::endl;      
        out << "  language: " << val.language << std::endl;      
        out << "  last_check_date: " << val.last_check_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  latitude: " << val.latitude << std::endl;      
        out << "  longitude: " << val.longitude << std::endl;      
        out << "  message_sent: " << val.message_sent << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  channel_namespace: " << val.channel_namespace << std::endl;      
        out << "  newsgate_category_name: " << val.newsgate_category_name << std::endl;      
        out << "  next_check_date: " << val.next_check_date << std::endl;      
        out << "  parent_channel_id: " << val.parent_channel_id << std::endl;      
        out << "  qa_date: " << val.qa_date << std::endl;      
        out << "  qa_description: " << val.qa_description << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  qa_user_id: " << val.qa_user_id << std::endl;      
        out << "  radius: " << val.radius << std::endl;      
        out << "  radius_units: " << val.radius_units << std::endl;      
        out << "  size_id: " << val.size_id << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  status_change_date: " << val.status_change_date << std::endl;      
        out << "  superseded_by_channel_id: " << val.superseded_by_channel_id << std::endl;      
        out << "  trigger_type: " << val.trigger_type << std::endl;      
        out << "  triggers_status: " << val.triggers_status << std::endl;      
        out << "  triggers_version: " << val.triggers_version << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  visibility: " << val.visibility << std::endl;      
        return out;      
      }      
      ORMObjectMember ChannelInventory::members_[11] = {      
        { "active_user_count", &(null<ChannelInventory>()->active_user_count), 0},        
        { "hits", &(null<ChannelInventory>()->hits), 0},        
        { "hits_kws", &(null<ChannelInventory>()->hits_kws), 0},        
        { "hits_search_kws", &(null<ChannelInventory>()->hits_search_kws), 0},        
        { "hits_url_kws", &(null<ChannelInventory>()->hits_url_kws), 0},        
        { "hits_urls", &(null<ChannelInventory>()->hits_urls), 0},        
        { "sum_ecpm", &(null<ChannelInventory>()->sum_ecpm), 0},        
        { "total_user_count", &(null<ChannelInventory>()->total_user_count), 0},        
        { "sdate", &(null<ChannelInventory>()->sdate_), 0},        
        { "channel_id", &(null<ChannelInventory>()->channel_id_), 0},        
        { "colo_id", &(null<ChannelInventory>()->colo_id_), 0},        
      };
      
      ChannelInventory::ChannelInventory (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      ChannelInventory::ChannelInventory (DB::IConn& connection,      
        const ORMDate::value_type& sdate,        
        const ORMInt::value_type& channel_id,        
        const ORMInt::value_type& colo_id        
      )      
        :ORMObject<postgres_connection>(connection), sdate_(sdate), channel_id_(channel_id), colo_id_(colo_id)
      {}
      
      ChannelInventory::ChannelInventory(const ChannelInventory& from)      
        :ORMObject<postgres_connection>(from), sdate_(from.sdate_), channel_id_(from.channel_id_), colo_id_(from.colo_id_),
        active_user_count(from.active_user_count),      
        hits(from.hits),      
        hits_kws(from.hits_kws),      
        hits_search_kws(from.hits_search_kws),      
        hits_url_kws(from.hits_url_kws),      
        hits_urls(from.hits_urls),      
        sum_ecpm(from.sum_ecpm),      
        total_user_count(from.total_user_count)      
      {      
      }
      
      ChannelInventory& ChannelInventory::operator=(const ChannelInventory& from)      
      {      
        Unused(from);      
        active_user_count = from.active_user_count;      
        hits = from.hits;      
        hits_kws = from.hits_kws;      
        hits_search_kws = from.hits_search_kws;      
        hits_url_kws = from.hits_url_kws;      
        hits_urls = from.hits_urls;      
        sum_ecpm = from.sum_ecpm;      
        total_user_count = from.total_user_count;      
        return *this;      
      }
      
      bool ChannelInventory::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "active_user_count,"      
            "hits,"      
            "hits_kws,"      
            "hits_search_kws,"      
            "hits_url_kws,"      
            "hits_urls,"      
            "sum_ecpm,"      
            "total_user_count "      
          "FROM channelinventory "      
          "WHERE sdate = :i1 AND channel_id = :i2 AND colo_id = :i3"));      
        query  << sdate_ << channel_id_ << colo_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool ChannelInventory::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE channelinventory SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE sdate = :i1 AND channel_id = :i2 AND colo_id = :i3";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << sdate_ << channel_id_ << colo_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool ChannelInventory::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (sdate_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(sdate) + 1 FROM channelinventory"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> sdate_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO channelinventory (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " sdate, channel_id , colo_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2, :i3)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << sdate_ << channel_id_ << colo_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool ChannelInventory::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM channelinventory WHERE sdate = :i1 AND channel_id = :i2 AND colo_id = :i3"));      
        query  << sdate_ << channel_id_ << colo_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool ChannelInventory::del ()      
      {      
        return delet();      
      }
      
      void ChannelInventory::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: ChannelInventory" << std::endl;      
          out << "{" << std::endl;      
          out << "* sdate = " << strof(sdate_) << ";" << std::endl;      
          out << "* channel_id = " << strof(channel_id_) << ";" << std::endl;      
          out << "* colo_id = " << strof(colo_id_) << ";" << std::endl;      
          out << "  active_user_count = " << strof(active_user_count) << ";" << std::endl;      
          out << "  hits = " << strof(hits) << ";" << std::endl;      
          out << "  hits_kws = " << strof(hits_kws) << ";" << std::endl;      
          out << "  hits_search_kws = " << strof(hits_search_kws) << ";" << std::endl;      
          out << "  hits_url_kws = " << strof(hits_url_kws) << ";" << std::endl;      
          out << "  hits_urls = " << strof(hits_urls) << ";" << std::endl;      
          out << "  sum_ecpm = " << strof(sum_ecpm) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log ChannelInventory because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const ChannelInventory& val)      
      {      
        out << "ChannelInventory:" << std::endl;      
        out << " *sdate: " << val.sdate_ << std::endl;      
        out << " *channel_id: " << val.channel_id_ << std::endl;      
        out << " *colo_id: " << val.colo_id_ << std::endl;      
        out << "  active_user_count: " << val.active_user_count << std::endl;      
        out << "  hits: " << val.hits << std::endl;      
        out << "  hits_kws: " << val.hits_kws << std::endl;      
        out << "  hits_search_kws: " << val.hits_search_kws << std::endl;      
        out << "  hits_url_kws: " << val.hits_url_kws << std::endl;      
        out << "  hits_urls: " << val.hits_urls << std::endl;      
        out << "  sum_ecpm: " << val.sum_ecpm << std::endl;      
        out << "  total_user_count: " << val.total_user_count << std::endl;      
        return out;      
      }      
      ORMObjectMember Channelrate::members_[8] = {      
        { "channel_id", &(null<Channelrate>()->channel_id), 0},        
        { "cpc", &(null<Channelrate>()->cpc), 0},        
        { "cpm", &(null<Channelrate>()->cpm), 0},        
        { "currency_id", &(null<Channelrate>()->currency_id), 0},        
        { "effective_date", &(null<Channelrate>()->effective_date), 0},        
        { "last_updated", &(null<Channelrate>()->last_updated), 0},        
        { "rate_type", &(null<Channelrate>()->rate_type), 0},        
        { "channel_rate_id", &(null<Channelrate>()->channel_rate_id_), 0},        
      };
      
      Channelrate::Channelrate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Channelrate::Channelrate (DB::IConn& connection,      
        const ORMInt::value_type& channel_rate_id        
      )      
        :ORMObject<postgres_connection>(connection), channel_rate_id_(channel_rate_id)
      {}
      
      Channelrate::Channelrate(const Channelrate& from)      
        :ORMObject<postgres_connection>(from), channel_rate_id_(from.channel_rate_id_),
        channel_id(from.channel_id),      
        cpc(from.cpc),      
        cpm(from.cpm),      
        currency_id(from.currency_id),      
        effective_date(from.effective_date),      
        last_updated(from.last_updated),      
        rate_type(from.rate_type)      
      {      
      }
      
      Channelrate& Channelrate::operator=(const Channelrate& from)      
      {      
        Unused(from);      
        channel_rate_id_ = from.channel_rate_id_;      
        channel_id = from.channel_id;      
        cpc = from.cpc;      
        cpm = from.cpm;      
        currency_id = from.currency_id;      
        effective_date = from.effective_date;      
        last_updated = from.last_updated;      
        rate_type = from.rate_type;      
        return *this;      
      }
      
      bool Channelrate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "channel_id,"      
            "cpc,"      
            "cpm,"      
            "currency_id,"      
            "effective_date,"      
            "last_updated,"      
            "rate_type "      
          "FROM channelrate "      
          "WHERE channel_rate_id = :i1"));      
        query  << channel_rate_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Channelrate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE channelrate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE channel_rate_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << channel_rate_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channelrate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (channel_rate_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('channelrate_channel_rate_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> channel_rate_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO channelrate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " channel_rate_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << channel_rate_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channelrate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM channelrate WHERE channel_rate_id = :i1"));      
        query  << channel_rate_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Channelrate::del ()      
      {      
        return delet();      
      }
      
      void Channelrate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Channelrate" << std::endl;      
          out << "{" << std::endl;      
          out << "* channel_rate_id = " << strof(channel_rate_id_) << ";" << std::endl;      
          out << "  channel_id = " << strof(channel_id) << ";" << std::endl;      
          out << "  cpc = " << strof(cpc) << ";" << std::endl;      
          out << "  cpm = " << strof(cpm) << ";" << std::endl;      
          out << "  currency_id = " << strof(currency_id) << ";" << std::endl;      
          out << "  effective_date = " << strof(effective_date) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Channelrate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Channelrate& val)      
      {      
        out << "Channelrate:" << std::endl;      
        out << " *channel_rate_id: " << val.channel_rate_id_ << std::endl;      
        out << "  channel_id: " << val.channel_id << std::endl;      
        out << "  cpc: " << val.cpc << std::endl;      
        out << "  cpm: " << val.cpm << std::endl;      
        out << "  currency_id: " << val.currency_id << std::endl;      
        out << "  effective_date: " << val.effective_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  rate_type: " << val.rate_type << std::endl;      
        return out;      
      }      
      ORMObjectMember Channeltrigger::members_[12] = {      
        { "channel_id", &(null<Channeltrigger>()->channel_id), 0},        
        { "channel_type", &(null<Channeltrigger>()->channel_type), 0},        
        { "country_code", &(null<Channeltrigger>()->country_code), 0},        
        { "last_updated", &(null<Channeltrigger>()->last_updated), 0},        
        { "masked", &(null<Channeltrigger>()->masked), 0},        
        { "negative", &(null<Channeltrigger>()->negative), "'N'"},        
        { "original_trigger", &(null<Channeltrigger>()->original_trigger), 0},        
        { "qa_status", &(null<Channeltrigger>()->qa_status), 0},        
        { "trigger_group", &(null<Channeltrigger>()->trigger_group), 0},        
        { "trigger_id", &(null<Channeltrigger>()->trigger_id), 0},        
        { "trigger_type", &(null<Channeltrigger>()->trigger_type), 0},        
        { "channel_trigger_id", &(null<Channeltrigger>()->channel_trigger_id_), 0},        
      };
      
      Channeltrigger::Channeltrigger (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Channeltrigger::Channeltrigger (DB::IConn& connection,      
        const ORMInt::value_type& channel_trigger_id        
      )      
        :ORMObject<postgres_connection>(connection), channel_trigger_id_(channel_trigger_id)
      {}
      
      Channeltrigger::Channeltrigger(const Channeltrigger& from)      
        :ORMObject<postgres_connection>(from), channel_trigger_id_(from.channel_trigger_id_),
        channel_id(from.channel_id),      
        channel_type(from.channel_type),      
        country_code(from.country_code),      
        last_updated(from.last_updated),      
        masked(from.masked),      
        negative(from.negative),      
        original_trigger(from.original_trigger),      
        qa_status(from.qa_status),      
        trigger_group(from.trigger_group),      
        trigger_id(from.trigger_id),      
        trigger_type(from.trigger_type)      
      {      
      }
      
      Channeltrigger& Channeltrigger::operator=(const Channeltrigger& from)      
      {      
        Unused(from);      
        channel_trigger_id_ = from.channel_trigger_id_;      
        channel_id = from.channel_id;      
        channel_type = from.channel_type;      
        country_code = from.country_code;      
        last_updated = from.last_updated;      
        masked = from.masked;      
        negative = from.negative;      
        original_trigger = from.original_trigger;      
        qa_status = from.qa_status;      
        trigger_group = from.trigger_group;      
        trigger_id = from.trigger_id;      
        trigger_type = from.trigger_type;      
        return *this;      
      }
      
      bool Channeltrigger::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "channel_id,"      
            "channel_type,"      
            "country_code,"      
            "last_updated,"      
            "masked,"      
            "negative,"      
            "original_trigger,"      
            "qa_status,"      
            "trigger_group,"      
            "trigger_id,"      
            "trigger_type "      
          "FROM channeltrigger "      
          "WHERE channel_trigger_id = :i1"));      
        query  << channel_trigger_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Channeltrigger::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE channeltrigger SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE channel_trigger_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << channel_trigger_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channeltrigger::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (channel_trigger_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('channeltrigger_channel_trigger_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> channel_trigger_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO channeltrigger (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " channel_trigger_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << channel_trigger_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Channeltrigger::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM channeltrigger WHERE channel_trigger_id = :i1"));      
        query  << channel_trigger_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Channeltrigger::del ()      
      {      
        return delet();      
      }
      
      void Channeltrigger::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Channeltrigger" << std::endl;      
          out << "{" << std::endl;      
          out << "* channel_trigger_id = " << strof(channel_trigger_id_) << ";" << std::endl;      
          out << "  channel_id = " << strof(channel_id) << ";" << std::endl;      
          out << "  channel_type = " << strof(channel_type) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  masked = " << strof(masked) << ";" << std::endl;      
          out << "  negative = " << strof(negative) << ";" << std::endl;      
          out << "  original_trigger = " << strof(original_trigger) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  trigger_group = " << strof(trigger_group) << ";" << std::endl;      
          out << "  trigger_id = " << strof(trigger_id) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Channeltrigger because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Channeltrigger& val)      
      {      
        out << "Channeltrigger:" << std::endl;      
        out << " *channel_trigger_id: " << val.channel_trigger_id_ << std::endl;      
        out << "  channel_id: " << val.channel_id << std::endl;      
        out << "  channel_type: " << val.channel_type << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  masked: " << val.masked << std::endl;      
        out << "  negative: " << val.negative << std::endl;      
        out << "  original_trigger: " << val.original_trigger << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  trigger_group: " << val.trigger_group << std::endl;      
        out << "  trigger_id: " << val.trigger_id << std::endl;      
        out << "  trigger_type: " << val.trigger_type << std::endl;      
        return out;      
      }      
      ORMObjectMember Colocation::members_[8] = {      
        { "account_id", &(null<Colocation>()->account), 0},        
        { "colo_rate_id", &(null<Colocation>()->colo_rate), 0},        
        { "last_updated", &(null<Colocation>()->last_updated), 0},        
        { "name", &(null<Colocation>()->name), 0},        
        { "optout_serving", &(null<Colocation>()->optout_serving), "'NON_OPTOUT'"},        
        { "status", &(null<Colocation>()->status), 0},        
        { "version", &(null<Colocation>()->version), 0},        
        { "colo_id", &(null<Colocation>()->id_), 0},        
      };
      
      Colocation::Colocation (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Colocation::Colocation (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Colocation::Colocation(const Colocation& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        account(from.account),      
        colo_rate(from.colo_rate),      
        last_updated(from.last_updated),      
        name(from.name),      
        optout_serving(from.optout_serving),      
        status(from.status),      
        version(from.version)      
      {      
      }
      
      Colocation& Colocation::operator=(const Colocation& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        account = from.account;      
        colo_rate = from.colo_rate;      
        last_updated = from.last_updated;      
        name = from.name;      
        optout_serving = from.optout_serving;      
        status = from.status;      
        version = from.version;      
        return *this;      
      }
      
      bool Colocation::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE colocation SET VERSION = now()";      
        {      
          strm << " WHERE colo_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colocation::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "colo_rate_id,"      
            "last_updated,"      
            "name,"      
            "optout_serving,"      
            "status,"      
            "version "      
          "FROM colocation "      
          "WHERE colo_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Colocation::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE colocation SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE colo_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colocation::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('colocation_colo_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO colocation (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 7; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " colo_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 7; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colocation::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM colocation WHERE colo_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Colocation::del ()      
      {      
        DB::Query  query(conn.get_query("UPDATE colocation SET"      
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE colo_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      void Colocation::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Colocation" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  account = " << strof(account) << ";" << std::endl;      
          out << "  colo_rate = " << strof(colo_rate) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  optout_serving = " << strof(optout_serving) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Colocation because not select");      
        }      
      }
      
      bool Colocation::has_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query query(conn.get_query("SELECT "      
            "colo_id "      
          "FROM colocation "      
          "WHERE name = :i1"));      
        query.set(name);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> this->id_;      
          return true;      
        }      
        return false;      
      }
      
      bool Colocation::select_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "colo_rate_id,"      
            "last_updated,"      
            "optout_serving,"      
            "status,"      
            "version,"      
            "colo_id "      
          "FROM colocation "      
          "WHERE name = :i1"));      
        query.set(name);      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          result >> account      
                 >> colo_rate      
                 >> last_updated      
                 >> optout_serving      
                 >> status      
                 >> version      
                 >> id_;      
          return true;      
        }      
        return false;      
      }
      
      bool Colocation::update_name (const ORMString::value_type& name, bool set_defaults )      
      {      
        Unused(name);        
        Unused(set_defaults);        
        this->name = name;        
        std::ostringstream strm;      
        strm << "UPDATE colocation SET ";      
        int counter = 1;      
          counter = update_(strm, counter, this, members_[0], set_defaults);      
          counter = update_(strm, counter, this, members_[1], set_defaults);      
          counter = update_(strm, counter, this, members_[2], set_defaults);      
          counter = update_(strm, counter, this, members_[4], set_defaults);      
          counter = update_(strm, counter, this, members_[5], set_defaults);      
          counter = update_(strm, counter, this, members_[6], set_defaults);      
        if(counter > 1)      
        {      
          strm << " WHERE name = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          setin_(query, account);      
          setin_(query, colo_rate);      
          setin_(query, last_updated);      
          setin_(query, optout_serving);      
          setin_(query, status);      
          setin_(query, version);      
          query.set(name);      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colocation::delet_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query  query(conn.get_query("DELETE FROM colocation WHERE name = :i1"));      
        query.set(name);      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Colocation::del_name (const ORMString::value_type& name)      
      {      
        Unused(name);        
        this->name = name;        
        DB::Query  query(conn.get_query("UPDATE colocation SET"       
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE name = :i1"));      
        query.set(name);      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      std::ostream& operator<< (std::ostream& out, const Colocation& val)      
      {      
        out << "Colocation:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  account: " << val.account << std::endl;      
        out << "  colo_rate: " << val.colo_rate << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  optout_serving: " << val.optout_serving << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember ColocationRate::members_[5] = {      
        { "colo_id", &(null<ColocationRate>()->colo), 0},        
        { "effective_date", &(null<ColocationRate>()->effective_date), "now()"},        
        { "last_updated", &(null<ColocationRate>()->last_updated), 0},        
        { "revenue_share", &(null<ColocationRate>()->revenue_share), 0},        
        { "colo_rate_id", &(null<ColocationRate>()->id_), 0},        
      };
      
      ColocationRate::ColocationRate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      ColocationRate::ColocationRate (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      ColocationRate::ColocationRate(const ColocationRate& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        colo(from.colo),      
        effective_date(from.effective_date),      
        last_updated(from.last_updated),      
        revenue_share(from.revenue_share)      
      {      
      }
      
      ColocationRate& ColocationRate::operator=(const ColocationRate& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        colo = from.colo;      
        effective_date = from.effective_date;      
        last_updated = from.last_updated;      
        revenue_share = from.revenue_share;      
        return *this;      
      }
      
      bool ColocationRate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "colo_id,"      
            "effective_date,"      
            "last_updated,"      
            "revenue_share "      
          "FROM colocationrate "      
          "WHERE colo_rate_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool ColocationRate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE colocationrate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE colo_rate_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool ColocationRate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('colocationrate_colo_rate_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO colocationrate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " colo_rate_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool ColocationRate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM colocationrate WHERE colo_rate_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool ColocationRate::del ()      
      {      
        return delet();      
      }
      
      void ColocationRate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: ColocationRate" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  colo = " << strof(colo) << ";" << std::endl;      
          out << "  effective_date = " << strof(effective_date) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log ColocationRate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const ColocationRate& val)      
      {      
        out << "ColocationRate:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  colo: " << val.colo << std::endl;      
        out << "  effective_date: " << val.effective_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  revenue_share: " << val.revenue_share << std::endl;      
        return out;      
      }      
      ORMObjectMember Colostats::members_[5] = {      
        { "last_campaign_update", &(null<Colostats>()->last_campaign_update), 0},        
        { "last_channel_update", &(null<Colostats>()->last_channel_update), 0},        
        { "last_stats_upload", &(null<Colostats>()->last_stats_upload), 0},        
        { "software_version", &(null<Colostats>()->software_version), 0},        
        { "colo_id", &(null<Colostats>()->colo_id_), 0},        
      };
      
      Colostats::Colostats (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Colostats::Colostats (DB::IConn& connection,      
        const ORMInt::value_type& colo_id        
      )      
        :ORMObject<postgres_connection>(connection), colo_id_(colo_id)
      {}
      
      Colostats::Colostats(const Colostats& from)      
        :ORMObject<postgres_connection>(from), colo_id_(from.colo_id_),
        last_campaign_update(from.last_campaign_update),      
        last_channel_update(from.last_channel_update),      
        last_stats_upload(from.last_stats_upload),      
        software_version(from.software_version)      
      {      
      }
      
      Colostats& Colostats::operator=(const Colostats& from)      
      {      
        Unused(from);      
        last_campaign_update = from.last_campaign_update;      
        last_channel_update = from.last_channel_update;      
        last_stats_upload = from.last_stats_upload;      
        software_version = from.software_version;      
        return *this;      
      }
      
      bool Colostats::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_campaign_update,"      
            "last_channel_update,"      
            "last_stats_upload,"      
            "software_version "      
          "FROM colostats "      
          "WHERE colo_id = :i1"));      
        query  << colo_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Colostats::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE colostats SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE colo_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << colo_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colostats::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (colo_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(colo_id) + 1 FROM colostats"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> colo_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO colostats (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " colo_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << colo_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Colostats::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM colostats WHERE colo_id = :i1"));      
        query  << colo_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Colostats::del ()      
      {      
        return delet();      
      }
      
      void Colostats::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Colostats" << std::endl;      
          out << "{" << std::endl;      
          out << "* colo_id = " << strof(colo_id_) << ";" << std::endl;      
          out << "  last_campaign_update = " << strof(last_campaign_update) << ";" << std::endl;      
          out << "  last_channel_update = " << strof(last_channel_update) << ";" << std::endl;      
          out << "  last_stats_upload = " << strof(last_stats_upload) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Colostats because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Colostats& val)      
      {      
        out << "Colostats:" << std::endl;      
        out << " *colo_id: " << val.colo_id_ << std::endl;      
        out << "  last_campaign_update: " << val.last_campaign_update << std::endl;      
        out << "  last_channel_update: " << val.last_channel_update << std::endl;      
        out << "  last_stats_upload: " << val.last_stats_upload << std::endl;      
        out << "  software_version: " << val.software_version << std::endl;      
        return out;      
      }      
      ORMObjectMember Country::members_[25] = {      
        { "ad_footer_url", &(null<Country>()->ad_footer_url), 0},        
        { "ad_tag_domain", &(null<Country>()->ad_tag_domain), 0},        
        { "adserving_domain", &(null<Country>()->adserving_domain), 0},        
        { "conversion_tag_domain", &(null<Country>()->conversion_tag_domain), 0},        
        { "country_id", &(null<Country>()->country_id), 0},        
        { "currency_id", &(null<Country>()->currency_id), 0},        
        { "default_agency_commission", &(null<Country>()->default_agency_commission), 0},        
        { "default_payment_terms", &(null<Country>()->default_payment_terms), 0},        
        { "default_vat_rate", &(null<Country>()->default_vat_rate), 0},        
        { "discover_domain", &(null<Country>()->discover_domain), 0},        
        { "high_channel_threshold", &(null<Country>()->high_channel_threshold), 0},        
        { "invoice_custom_report_id", &(null<Country>()->invoice_custom_report_id), 0},        
        { "language", &(null<Country>()->language), 0},        
        { "last_updated", &(null<Country>()->last_updated), 0},        
        { "low_channel_threshold", &(null<Country>()->low_channel_threshold), 0},        
        { "max_url_trigger_share", &(null<Country>()->max_url_trigger_share), 0},        
        { "min_tag_visibility", &(null<Country>()->min_tag_visibility), 0},        
        { "min_url_trigger_threshold", &(null<Country>()->min_url_trigger_threshold), 0},        
        { "sortorder", &(null<Country>()->sortorder), 0},        
        { "static_domain", &(null<Country>()->static_domain), 0},        
        { "timezone_id", &(null<Country>()->timezone_id), 0},        
        { "vat_enabled", &(null<Country>()->vat_enabled), 0},        
        { "vat_number_input_enabled", &(null<Country>()->vat_number_input_enabled), 0},        
        { "version", &(null<Country>()->version), 0},        
        { "country_code", &(null<Country>()->country_code_), 0},        
      };
      
      Country::Country (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Country::Country (DB::IConn& connection,      
        const ORMString::value_type& country_code        
      )      
        :ORMObject<postgres_connection>(connection), country_code_(country_code)
      {}
      
      Country::Country(const Country& from)      
        :ORMObject<postgres_connection>(from), country_code_(from.country_code_),
        ad_footer_url(from.ad_footer_url),      
        ad_tag_domain(from.ad_tag_domain),      
        adserving_domain(from.adserving_domain),      
        conversion_tag_domain(from.conversion_tag_domain),      
        country_id(from.country_id),      
        currency_id(from.currency_id),      
        default_agency_commission(from.default_agency_commission),      
        default_payment_terms(from.default_payment_terms),      
        default_vat_rate(from.default_vat_rate),      
        discover_domain(from.discover_domain),      
        high_channel_threshold(from.high_channel_threshold),      
        invoice_custom_report_id(from.invoice_custom_report_id),      
        language(from.language),      
        last_updated(from.last_updated),      
        low_channel_threshold(from.low_channel_threshold),      
        max_url_trigger_share(from.max_url_trigger_share),      
        min_tag_visibility(from.min_tag_visibility),      
        min_url_trigger_threshold(from.min_url_trigger_threshold),      
        sortorder(from.sortorder),      
        static_domain(from.static_domain),      
        timezone_id(from.timezone_id),      
        vat_enabled(from.vat_enabled),      
        vat_number_input_enabled(from.vat_number_input_enabled),      
        version(from.version)      
      {      
      }
      
      Country& Country::operator=(const Country& from)      
      {      
        Unused(from);      
        country_code_ = from.country_code_;      
        ad_footer_url = from.ad_footer_url;      
        ad_tag_domain = from.ad_tag_domain;      
        adserving_domain = from.adserving_domain;      
        conversion_tag_domain = from.conversion_tag_domain;      
        country_id = from.country_id;      
        currency_id = from.currency_id;      
        default_agency_commission = from.default_agency_commission;      
        default_payment_terms = from.default_payment_terms;      
        default_vat_rate = from.default_vat_rate;      
        discover_domain = from.discover_domain;      
        high_channel_threshold = from.high_channel_threshold;      
        invoice_custom_report_id = from.invoice_custom_report_id;      
        language = from.language;      
        last_updated = from.last_updated;      
        low_channel_threshold = from.low_channel_threshold;      
        max_url_trigger_share = from.max_url_trigger_share;      
        min_tag_visibility = from.min_tag_visibility;      
        min_url_trigger_threshold = from.min_url_trigger_threshold;      
        sortorder = from.sortorder;      
        static_domain = from.static_domain;      
        timezone_id = from.timezone_id;      
        vat_enabled = from.vat_enabled;      
        vat_number_input_enabled = from.vat_number_input_enabled;      
        version = from.version;      
        return *this;      
      }
      
      bool Country::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE country SET VERSION = now()";      
        {      
          strm << " WHERE country_code = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << country_code_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Country::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "ad_footer_url,"      
            "ad_tag_domain,"      
            "adserving_domain,"      
            "conversion_tag_domain,"      
            "country_id,"      
            "currency_id,"      
            "default_agency_commission,"      
            "default_payment_terms,"      
            "default_vat_rate,"      
            "discover_domain,"      
            "high_channel_threshold,"      
            "invoice_custom_report_id,"      
            "language,"      
            "last_updated,"      
            "low_channel_threshold,"      
            "max_url_trigger_share,"      
            "min_tag_visibility,"      
            "min_url_trigger_threshold,"      
            "sortorder,"      
            "static_domain,"      
            "timezone_id,"      
            "vat_enabled,"      
            "vat_number_input_enabled,"      
            "version "      
          "FROM country "      
          "WHERE country_code = :i1"));      
        query  << country_code_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 24; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Country::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE country SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 24; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE country_code = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 24; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << country_code_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Country::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (country_code_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('country_country_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> country_code_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO country (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 24; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " country_code)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 24; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 24; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << country_code_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Country::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM country WHERE country_code = :i1"));      
        query  << country_code_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Country::del ()      
      {      
        return delet();      
      }
      
      void Country::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Country" << std::endl;      
          out << "{" << std::endl;      
          out << "* country_code = " << strof(country_code_) << ";" << std::endl;      
          out << "  ad_footer_url = " << strof(ad_footer_url) << ";" << std::endl;      
          out << "  ad_tag_domain = " << strof(ad_tag_domain) << ";" << std::endl;      
          out << "  adserving_domain = " << strof(adserving_domain) << ";" << std::endl;      
          out << "  conversion_tag_domain = " << strof(conversion_tag_domain) << ";" << std::endl;      
          out << "  country_id = " << strof(country_id) << ";" << std::endl;      
          out << "  currency_id = " << strof(currency_id) << ";" << std::endl;      
          out << "  default_agency_commission = " << strof(default_agency_commission) << ";" << std::endl;      
          out << "  default_payment_terms = " << strof(default_payment_terms) << ";" << std::endl;      
          out << "  default_vat_rate = " << strof(default_vat_rate) << ";" << std::endl;      
          out << "  discover_domain = " << strof(discover_domain) << ";" << std::endl;      
          out << "  high_channel_threshold = " << strof(high_channel_threshold) << ";" << std::endl;      
          out << "  invoice_custom_report_id = " << strof(invoice_custom_report_id) << ";" << std::endl;      
          out << "  language = " << strof(language) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  low_channel_threshold = " << strof(low_channel_threshold) << ";" << std::endl;      
          out << "  max_url_trigger_share = " << strof(max_url_trigger_share) << ";" << std::endl;      
          out << "  min_tag_visibility = " << strof(min_tag_visibility) << ";" << std::endl;      
          out << "  min_url_trigger_threshold = " << strof(min_url_trigger_threshold) << ";" << std::endl;      
          out << "  sortorder = " << strof(sortorder) << ";" << std::endl;      
          out << "  static_domain = " << strof(static_domain) << ";" << std::endl;      
          out << "  timezone_id = " << strof(timezone_id) << ";" << std::endl;      
          out << "  vat_enabled = " << strof(vat_enabled) << ";" << std::endl;      
          out << "  vat_number_input_enabled = " << strof(vat_number_input_enabled) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Country because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Country& val)      
      {      
        out << "Country:" << std::endl;      
        out << " *country_code: " << val.country_code_ << std::endl;      
        out << "  ad_footer_url: " << val.ad_footer_url << std::endl;      
        out << "  ad_tag_domain: " << val.ad_tag_domain << std::endl;      
        out << "  adserving_domain: " << val.adserving_domain << std::endl;      
        out << "  conversion_tag_domain: " << val.conversion_tag_domain << std::endl;      
        out << "  country_id: " << val.country_id << std::endl;      
        out << "  currency_id: " << val.currency_id << std::endl;      
        out << "  default_agency_commission: " << val.default_agency_commission << std::endl;      
        out << "  default_payment_terms: " << val.default_payment_terms << std::endl;      
        out << "  default_vat_rate: " << val.default_vat_rate << std::endl;      
        out << "  discover_domain: " << val.discover_domain << std::endl;      
        out << "  high_channel_threshold: " << val.high_channel_threshold << std::endl;      
        out << "  invoice_custom_report_id: " << val.invoice_custom_report_id << std::endl;      
        out << "  language: " << val.language << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  low_channel_threshold: " << val.low_channel_threshold << std::endl;      
        out << "  max_url_trigger_share: " << val.max_url_trigger_share << std::endl;      
        out << "  min_tag_visibility: " << val.min_tag_visibility << std::endl;      
        out << "  min_url_trigger_threshold: " << val.min_url_trigger_threshold << std::endl;      
        out << "  sortorder: " << val.sortorder << std::endl;      
        out << "  static_domain: " << val.static_domain << std::endl;      
        out << "  timezone_id: " << val.timezone_id << std::endl;      
        out << "  vat_enabled: " << val.vat_enabled << std::endl;      
        out << "  vat_number_input_enabled: " << val.vat_number_input_enabled << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Creative::members_[17] = {      
        { "account_id", &(null<Creative>()->account), 0},        
        { "display_status_id", &(null<Creative>()->display_status_id), "1"},        
        { "expandable", &(null<Creative>()->expandable), "'N'"},        
        { "expansion", &(null<Creative>()->expansion), 0},        
        { "flags", &(null<Creative>()->flags), 0},        
        { "last_deactivated", &(null<Creative>()->last_deactivated), 0},        
        { "last_updated", &(null<Creative>()->last_updated), 0},        
        { "name", &(null<Creative>()->name), 0},        
        { "qa_date", &(null<Creative>()->qa_date), 0},        
        { "qa_description", &(null<Creative>()->qa_description), 0},        
        { "qa_status", &(null<Creative>()->qa_status), 0},        
        { "qa_user_id", &(null<Creative>()->qa_user_id), 0},        
        { "size_id", &(null<Creative>()->size), 0},        
        { "status", &(null<Creative>()->status), 0},        
        { "template_id", &(null<Creative>()->template_id), 0},        
        { "version", &(null<Creative>()->version), 0},        
        { "creative_id", &(null<Creative>()->id_), 0},        
      };
      
      Creative::Creative (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Creative::Creative (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Creative::Creative(const Creative& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        account(from.account),      
        display_status_id(from.display_status_id),      
        expandable(from.expandable),      
        expansion(from.expansion),      
        flags(from.flags),      
        last_deactivated(from.last_deactivated),      
        last_updated(from.last_updated),      
        name(from.name),      
        qa_date(from.qa_date),      
        qa_description(from.qa_description),      
        qa_status(from.qa_status),      
        qa_user_id(from.qa_user_id),      
        size(from.size),      
        status(from.status),      
        template_id(from.template_id),      
        version(from.version)      
      {      
      }
      
      Creative& Creative::operator=(const Creative& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        account = from.account;      
        display_status_id = from.display_status_id;      
        expandable = from.expandable;      
        expansion = from.expansion;      
        flags = from.flags;      
        last_deactivated = from.last_deactivated;      
        last_updated = from.last_updated;      
        name = from.name;      
        qa_date = from.qa_date;      
        qa_description = from.qa_description;      
        qa_status = from.qa_status;      
        qa_user_id = from.qa_user_id;      
        size = from.size;      
        status = from.status;      
        template_id = from.template_id;      
        version = from.version;      
        return *this;      
      }
      
      bool Creative::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE creative SET VERSION = now()";      
        {      
          strm << " WHERE creative_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creative::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "display_status_id,"      
            "expandable,"      
            "expansion,"      
            "flags,"      
            "last_deactivated,"      
            "last_updated,"      
            "name,"      
            "qa_date,"      
            "qa_description,"      
            "qa_status,"      
            "qa_user_id,"      
            "size_id,"      
            "status,"      
            "template_id,"      
            "version "      
          "FROM creative "      
          "WHERE creative_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Creative::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creative SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 16; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creative::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('creative_creative_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creative (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 16; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creative::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creative WHERE creative_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Creative::del ()      
      {      
        int status_id = get_display_status_id(conn, "creative", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE creative SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE creative_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Creative::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "creative", status);      
        DB::Query  query(conn.get_query("UPDATE creative SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE creative_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Creative::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Creative" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  account = " << strof(account) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  expandable = " << strof(expandable) << ";" << std::endl;      
          out << "  expansion = " << strof(expansion) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  last_deactivated = " << strof(last_deactivated) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  qa_date = " << strof(qa_date) << ";" << std::endl;      
          out << "  qa_description = " << strof(qa_description) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  qa_user_id = " << strof(qa_user_id) << ";" << std::endl;      
          out << "  size = " << strof(size) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  template_id = " << strof(template_id) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Creative because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Creative& val)      
      {      
        out << "Creative:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  account: " << val.account << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  expandable: " << val.expandable << std::endl;      
        out << "  expansion: " << val.expansion << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  last_deactivated: " << val.last_deactivated << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  qa_date: " << val.qa_date << std::endl;      
        out << "  qa_description: " << val.qa_description << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  qa_user_id: " << val.qa_user_id << std::endl;      
        out << "  size: " << val.size << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  template_id: " << val.template_id << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember CreativeCategory::members_[6] = {      
        { "cct_id", &(null<CreativeCategory>()->type), 0},        
        { "last_updated", &(null<CreativeCategory>()->last_updated), 0},        
        { "name", &(null<CreativeCategory>()->name), 0},        
        { "qa_status", &(null<CreativeCategory>()->qa_status), "'A'"},        
        { "version", &(null<CreativeCategory>()->version), 0},        
        { "creative_category_id", &(null<CreativeCategory>()->id_), 0},        
      };
      
      CreativeCategory::CreativeCategory (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CreativeCategory::CreativeCategory (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CreativeCategory::CreativeCategory(const CreativeCategory& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        type(from.type),      
        last_updated(from.last_updated),      
        name(from.name),      
        qa_status(from.qa_status),      
        version(from.version)      
      {      
      }
      
      CreativeCategory& CreativeCategory::operator=(const CreativeCategory& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        type = from.type;      
        last_updated = from.last_updated;      
        name = from.name;      
        qa_status = from.qa_status;      
        version = from.version;      
        return *this;      
      }
      
      bool CreativeCategory::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE creativecategory SET VERSION = now()";      
        {      
          strm << " WHERE creative_category_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeCategory::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "cct_id,"      
            "last_updated,"      
            "name,"      
            "qa_status,"      
            "version "      
          "FROM creativecategory "      
          "WHERE creative_category_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CreativeCategory::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creativecategory SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_category_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeCategory::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('creativecategory_creative_category_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creativecategory (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_category_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeCategory::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creativecategory WHERE creative_category_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CreativeCategory::del ()      
      {      
        return delet();      
      }
      
      void CreativeCategory::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CreativeCategory" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  type = " << strof(type) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CreativeCategory because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CreativeCategory& val)      
      {      
        out << "CreativeCategory:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  type: " << val.type << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Creativecategorytype::members_[4] = {      
        { "last_updated", &(null<Creativecategorytype>()->last_updated), 0},        
        { "name", &(null<Creativecategorytype>()->name), 0},        
        { "version", &(null<Creativecategorytype>()->version), 0},        
        { "cct_id", &(null<Creativecategorytype>()->cct_id_), 0},        
      };
      
      Creativecategorytype::Creativecategorytype (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Creativecategorytype::Creativecategorytype (DB::IConn& connection,      
        const ORMInt::value_type& cct_id        
      )      
        :ORMObject<postgres_connection>(connection), cct_id_(cct_id)
      {}
      
      Creativecategorytype::Creativecategorytype(const Creativecategorytype& from)      
        :ORMObject<postgres_connection>(from), cct_id_(from.cct_id_),
        last_updated(from.last_updated),      
        name(from.name),      
        version(from.version)      
      {      
      }
      
      Creativecategorytype& Creativecategorytype::operator=(const Creativecategorytype& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        name = from.name;      
        version = from.version;      
        return *this;      
      }
      
      bool Creativecategorytype::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE creativecategorytype SET VERSION = now()";      
        {      
          strm << " WHERE cct_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << cct_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativecategorytype::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "name,"      
            "version "      
          "FROM creativecategorytype "      
          "WHERE cct_id = :i1"));      
        query  << cct_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Creativecategorytype::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creativecategorytype SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE cct_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << cct_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativecategorytype::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (cct_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(cct_id) + 1 FROM creativecategorytype"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> cct_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creativecategorytype (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " cct_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << cct_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativecategorytype::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creativecategorytype WHERE cct_id = :i1"));      
        query  << cct_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Creativecategorytype::del ()      
      {      
        return delet();      
      }
      
      void Creativecategorytype::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Creativecategorytype" << std::endl;      
          out << "{" << std::endl;      
          out << "* cct_id = " << strof(cct_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Creativecategorytype because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Creativecategorytype& val)      
      {      
        out << "Creativecategorytype:" << std::endl;      
        out << " *cct_id: " << val.cct_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember CreativeCategory_Creative::members_[3] = {      
        { "last_updated", &(null<CreativeCategory_Creative>()->last_updated), 0},        
        { "creative_category_id", &(null<CreativeCategory_Creative>()->creative_category_id_), 0},        
        { "creative_id", &(null<CreativeCategory_Creative>()->creative_id_), 0},        
      };
      
      CreativeCategory_Creative::CreativeCategory_Creative (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CreativeCategory_Creative::CreativeCategory_Creative (DB::IConn& connection,      
        const ORMInt::value_type& creative_category_id,        
        const ORMInt::value_type& creative_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_category_id_(creative_category_id), creative_id_(creative_id)
      {}
      
      CreativeCategory_Creative::CreativeCategory_Creative(const CreativeCategory_Creative& from)      
        :ORMObject<postgres_connection>(from), creative_category_id_(from.creative_category_id_), creative_id_(from.creative_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      CreativeCategory_Creative& CreativeCategory_Creative::operator=(const CreativeCategory_Creative& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool CreativeCategory_Creative::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM creativecategory_creative "      
          "WHERE creative_category_id = :i1 AND creative_id = :i2"));      
        query  << creative_category_id_ << creative_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CreativeCategory_Creative::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creativecategory_creative SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_category_id = :i1 AND creative_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << creative_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeCategory_Creative::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_category_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_category_id) + 1 FROM creativecategory_creative"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_category_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creativecategory_creative (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_category_id, creative_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << creative_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeCategory_Creative::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creativecategory_creative WHERE creative_category_id = :i1 AND creative_id = :i2"));      
        query  << creative_category_id_ << creative_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CreativeCategory_Creative::del ()      
      {      
        return delet();      
      }
      
      void CreativeCategory_Creative::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CreativeCategory_Creative" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_category_id = " << strof(creative_category_id_) << ";" << std::endl;      
          out << "* creative_id = " << strof(creative_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CreativeCategory_Creative because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CreativeCategory_Creative& val)      
      {      
        out << "CreativeCategory_Creative:" << std::endl;      
        out << " *creative_category_id: " << val.creative_category_id_ << std::endl;      
        out << " *creative_id: " << val.creative_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember CreativeOptionValue::members_[5] = {      
        { "last_updated", &(null<CreativeOptionValue>()->last_updated), 0},        
        { "value", &(null<CreativeOptionValue>()->value), 0},        
        { "version", &(null<CreativeOptionValue>()->version), 0},        
        { "creative_id", &(null<CreativeOptionValue>()->creative_id_), 0},        
        { "option_id", &(null<CreativeOptionValue>()->option_id_), 0},        
      };
      
      CreativeOptionValue::CreativeOptionValue (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CreativeOptionValue::CreativeOptionValue (DB::IConn& connection,      
        const ORMInt::value_type& creative_id,        
        const ORMInt::value_type& option_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_id_(creative_id), option_id_(option_id)
      {}
      
      CreativeOptionValue::CreativeOptionValue(const CreativeOptionValue& from)      
        :ORMObject<postgres_connection>(from), creative_id_(from.creative_id_), option_id_(from.option_id_),
        last_updated(from.last_updated),      
        value(from.value),      
        version(from.version)      
      {      
      }
      
      CreativeOptionValue& CreativeOptionValue::operator=(const CreativeOptionValue& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        value = from.value;      
        version = from.version;      
        return *this;      
      }
      
      bool CreativeOptionValue::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE creativeoptionvalue SET VERSION = now()";      
        {      
          strm << " WHERE creative_id = :i1 AND option_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << creative_id_ << option_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeOptionValue::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "value,"      
            "version "      
          "FROM creativeoptionvalue "      
          "WHERE creative_id = :i1 AND option_id = :i2"));      
        query  << creative_id_ << option_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CreativeOptionValue::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creativeoptionvalue SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_id = :i1 AND option_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << option_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeOptionValue::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_id) + 1 FROM creativeoptionvalue"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creativeoptionvalue (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_id, option_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << option_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CreativeOptionValue::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creativeoptionvalue WHERE creative_id = :i1 AND option_id = :i2"));      
        query  << creative_id_ << option_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CreativeOptionValue::del ()      
      {      
        return delet();      
      }
      
      void CreativeOptionValue::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CreativeOptionValue" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_id = " << strof(creative_id_) << ";" << std::endl;      
          out << "* option_id = " << strof(option_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  value = " << strof(value) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CreativeOptionValue because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CreativeOptionValue& val)      
      {      
        out << "CreativeOptionValue:" << std::endl;      
        out << " *creative_id: " << val.creative_id_ << std::endl;      
        out << " *option_id: " << val.option_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  value: " << val.value << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Creativesize::members_[12] = {      
        { "flags", &(null<Creativesize>()->flags), 0},        
        { "height", &(null<Creativesize>()->height), 0},        
        { "last_updated", &(null<Creativesize>()->last_updated), 0},        
        { "max_height", &(null<Creativesize>()->max_height), 0},        
        { "max_width", &(null<Creativesize>()->max_width), 0},        
        { "name", &(null<Creativesize>()->name), 0},        
        { "protocol_name", &(null<Creativesize>()->protocol_name), 0},        
        { "size_type_id", &(null<Creativesize>()->size_type_id), 0},        
        { "status", &(null<Creativesize>()->status), 0},        
        { "version", &(null<Creativesize>()->version), 0},        
        { "width", &(null<Creativesize>()->width), 0},        
        { "size_id", &(null<Creativesize>()->size_id_), 0},        
      };
      
      Creativesize::Creativesize (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Creativesize::Creativesize (DB::IConn& connection,      
        const ORMInt::value_type& size_id        
      )      
        :ORMObject<postgres_connection>(connection), size_id_(size_id)
      {}
      
      Creativesize::Creativesize(const Creativesize& from)      
        :ORMObject<postgres_connection>(from), size_id_(from.size_id_),
        flags(from.flags),      
        height(from.height),      
        last_updated(from.last_updated),      
        max_height(from.max_height),      
        max_width(from.max_width),      
        name(from.name),      
        protocol_name(from.protocol_name),      
        size_type_id(from.size_type_id),      
        status(from.status),      
        version(from.version),      
        width(from.width)      
      {      
      }
      
      Creativesize& Creativesize::operator=(const Creativesize& from)      
      {      
        Unused(from);      
        size_id_ = from.size_id_;      
        flags = from.flags;      
        height = from.height;      
        last_updated = from.last_updated;      
        max_height = from.max_height;      
        max_width = from.max_width;      
        name = from.name;      
        protocol_name = from.protocol_name;      
        size_type_id = from.size_type_id;      
        status = from.status;      
        version = from.version;      
        width = from.width;      
        return *this;      
      }
      
      bool Creativesize::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE creativesize SET VERSION = now()";      
        {      
          strm << " WHERE size_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << size_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativesize::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "flags,"      
            "height,"      
            "last_updated,"      
            "max_height,"      
            "max_width,"      
            "name,"      
            "protocol_name,"      
            "size_type_id,"      
            "status,"      
            "version,"      
            "width "      
          "FROM creativesize "      
          "WHERE size_id = :i1"));      
        query  << size_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Creativesize::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creativesize SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE size_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << size_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativesize::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (size_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('creativesize_size_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> size_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creativesize (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " size_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << size_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creativesize::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creativesize WHERE size_id = :i1"));      
        query  << size_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Creativesize::del ()      
      {      
        DB::Query  query(conn.get_query("UPDATE creativesize SET"      
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE size_id = :i1"));      
        query  << size_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      void Creativesize::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Creativesize" << std::endl;      
          out << "{" << std::endl;      
          out << "* size_id = " << strof(size_id_) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  height = " << strof(height) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  max_height = " << strof(max_height) << ";" << std::endl;      
          out << "  max_width = " << strof(max_width) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  protocol_name = " << strof(protocol_name) << ";" << std::endl;      
          out << "  size_type_id = " << strof(size_type_id) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Creativesize because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Creativesize& val)      
      {      
        out << "Creativesize:" << std::endl;      
        out << " *size_id: " << val.size_id_ << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  height: " << val.height << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  max_height: " << val.max_height << std::endl;      
        out << "  max_width: " << val.max_width << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  protocol_name: " << val.protocol_name << std::endl;      
        out << "  size_type_id: " << val.size_type_id << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  width: " << val.width << std::endl;      
        return out;      
      }      
      ORMObjectMember Creative_tagsize::members_[3] = {      
        { "last_updated", &(null<Creative_tagsize>()->last_updated), 0},        
        { "creative_id", &(null<Creative_tagsize>()->creative_id_), 0},        
        { "size_id", &(null<Creative_tagsize>()->size_id_), 0},        
      };
      
      Creative_tagsize::Creative_tagsize (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Creative_tagsize::Creative_tagsize (DB::IConn& connection,      
        const ORMInt::value_type& creative_id,        
        const ORMInt::value_type& size_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_id_(creative_id), size_id_(size_id)
      {}
      
      Creative_tagsize::Creative_tagsize(const Creative_tagsize& from)      
        :ORMObject<postgres_connection>(from), creative_id_(from.creative_id_), size_id_(from.size_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      Creative_tagsize& Creative_tagsize::operator=(const Creative_tagsize& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool Creative_tagsize::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM creative_tagsize "      
          "WHERE creative_id = :i1 AND size_id = :i2"));      
        query  << creative_id_ << size_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Creative_tagsize::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE creative_tagsize SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_id = :i1 AND size_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << size_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creative_tagsize::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_id) + 1 FROM creative_tagsize"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO creative_tagsize (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_id, size_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << size_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Creative_tagsize::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM creative_tagsize WHERE creative_id = :i1 AND size_id = :i2"));      
        query  << creative_id_ << size_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Creative_tagsize::del ()      
      {      
        return delet();      
      }
      
      void Creative_tagsize::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Creative_tagsize" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_id = " << strof(creative_id_) << ";" << std::endl;      
          out << "* size_id = " << strof(size_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Creative_tagsize because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Creative_tagsize& val)      
      {      
        out << "Creative_tagsize:" << std::endl;      
        out << " *creative_id: " << val.creative_id_ << std::endl;      
        out << " *size_id: " << val.size_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember Currency::members_[6] = {      
        { "currency_code", &(null<Currency>()->currency_code), 0},        
        { "fraction_digits", &(null<Currency>()->fraction_digits), 0},        
        { "last_updated", &(null<Currency>()->last_updated), 0},        
        { "source", &(null<Currency>()->source), 0},        
        { "version", &(null<Currency>()->version), 0},        
        { "currency_id", &(null<Currency>()->currency_id_), 0},        
      };
      
      Currency::Currency (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Currency::Currency (DB::IConn& connection,      
        const ORMInt::value_type& currency_id        
      )      
        :ORMObject<postgres_connection>(connection), currency_id_(currency_id)
      {}
      
      Currency::Currency(const Currency& from)      
        :ORMObject<postgres_connection>(from), currency_id_(from.currency_id_),
        currency_code(from.currency_code),      
        fraction_digits(from.fraction_digits),      
        last_updated(from.last_updated),      
        source(from.source),      
        version(from.version)      
      {      
      }
      
      Currency& Currency::operator=(const Currency& from)      
      {      
        Unused(from);      
        currency_id_ = from.currency_id_;      
        currency_code = from.currency_code;      
        fraction_digits = from.fraction_digits;      
        last_updated = from.last_updated;      
        source = from.source;      
        version = from.version;      
        return *this;      
      }
      
      bool Currency::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE currency SET VERSION = now()";      
        {      
          strm << " WHERE currency_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << currency_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Currency::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "currency_code,"      
            "fraction_digits,"      
            "last_updated,"      
            "source,"      
            "version "      
          "FROM currency "      
          "WHERE currency_id = :i1"));      
        query  << currency_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Currency::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE currency SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE currency_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << currency_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Currency::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (currency_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('currency_currency_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> currency_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO currency (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " currency_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << currency_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Currency::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM currency WHERE currency_id = :i1"));      
        query  << currency_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Currency::del ()      
      {      
        return delet();      
      }
      
      void Currency::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Currency" << std::endl;      
          out << "{" << std::endl;      
          out << "* currency_id = " << strof(currency_id_) << ";" << std::endl;      
          out << "  currency_code = " << strof(currency_code) << ";" << std::endl;      
          out << "  fraction_digits = " << strof(fraction_digits) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  source = " << strof(source) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Currency because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Currency& val)      
      {      
        out << "Currency:" << std::endl;      
        out << " *currency_id: " << val.currency_id_ << std::endl;      
        out << "  currency_code: " << val.currency_code << std::endl;      
        out << "  fraction_digits: " << val.fraction_digits << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  source: " << val.source << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember CurrencyExchange::members_[3] = {      
        { "effective_date", &(null<CurrencyExchange>()->effective_date), 0},        
        { "last_updated", &(null<CurrencyExchange>()->last_updated), 0},        
        { "currency_exchange_id", &(null<CurrencyExchange>()->id_), 0},        
      };
      
      CurrencyExchange::CurrencyExchange (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      CurrencyExchange::CurrencyExchange (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      CurrencyExchange::CurrencyExchange(const CurrencyExchange& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        effective_date(from.effective_date),      
        last_updated(from.last_updated)      
      {      
      }
      
      CurrencyExchange& CurrencyExchange::operator=(const CurrencyExchange& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        effective_date = from.effective_date;      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool CurrencyExchange::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "effective_date,"      
            "last_updated "      
          "FROM currencyexchange "      
          "WHERE currency_exchange_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool CurrencyExchange::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE currencyexchange SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE currency_exchange_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CurrencyExchange::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('currencyexchange_currency_exchange_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO currencyexchange (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " currency_exchange_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool CurrencyExchange::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM currencyexchange WHERE currency_exchange_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool CurrencyExchange::del ()      
      {      
        return delet();      
      }
      
      void CurrencyExchange::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: CurrencyExchange" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  effective_date = " << strof(effective_date) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log CurrencyExchange because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const CurrencyExchange& val)      
      {      
        out << "CurrencyExchange:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  effective_date: " << val.effective_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember Currencyexchangerate::members_[5] = {      
        { "last_updated", &(null<Currencyexchangerate>()->last_updated), 0},        
        { "last_updated_date", &(null<Currencyexchangerate>()->last_updated_date), 0},        
        { "rate", &(null<Currencyexchangerate>()->rate), 0},        
        { "currency_exchange_id", &(null<Currencyexchangerate>()->currency_exchange_id_), 0},        
        { "currency_id", &(null<Currencyexchangerate>()->currency_id_), 0},        
      };
      
      Currencyexchangerate::Currencyexchangerate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Currencyexchangerate::Currencyexchangerate (DB::IConn& connection,      
        const ORMInt::value_type& currency_exchange_id,        
        const ORMInt::value_type& currency_id        
      )      
        :ORMObject<postgres_connection>(connection), currency_exchange_id_(currency_exchange_id), currency_id_(currency_id)
      {}
      
      Currencyexchangerate::Currencyexchangerate(const Currencyexchangerate& from)      
        :ORMObject<postgres_connection>(from), currency_exchange_id_(from.currency_exchange_id_), currency_id_(from.currency_id_),
        last_updated(from.last_updated),      
        last_updated_date(from.last_updated_date),      
        rate(from.rate)      
      {      
      }
      
      Currencyexchangerate& Currencyexchangerate::operator=(const Currencyexchangerate& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        last_updated_date = from.last_updated_date;      
        rate = from.rate;      
        return *this;      
      }
      
      bool Currencyexchangerate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "last_updated_date,"      
            "rate "      
          "FROM currencyexchangerate "      
          "WHERE currency_exchange_id = :i1 AND currency_id = :i2"));      
        query  << currency_exchange_id_ << currency_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Currencyexchangerate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE currencyexchangerate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE currency_exchange_id = :i1 AND currency_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << currency_exchange_id_ << currency_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Currencyexchangerate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (currency_exchange_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(currency_exchange_id) + 1 FROM currencyexchangerate"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> currency_exchange_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO currencyexchangerate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 3; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " currency_exchange_id, currency_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 3; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << currency_exchange_id_ << currency_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Currencyexchangerate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM currencyexchangerate WHERE currency_exchange_id = :i1 AND currency_id = :i2"));      
        query  << currency_exchange_id_ << currency_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Currencyexchangerate::del ()      
      {      
        return delet();      
      }
      
      void Currencyexchangerate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Currencyexchangerate" << std::endl;      
          out << "{" << std::endl;      
          out << "* currency_exchange_id = " << strof(currency_exchange_id_) << ";" << std::endl;      
          out << "* currency_id = " << strof(currency_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  last_updated_date = " << strof(last_updated_date) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Currencyexchangerate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Currencyexchangerate& val)      
      {      
        out << "Currencyexchangerate:" << std::endl;      
        out << " *currency_exchange_id: " << val.currency_exchange_id_ << std::endl;      
        out << " *currency_id: " << val.currency_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  last_updated_date: " << val.last_updated_date << std::endl;      
        out << "  rate: " << val.rate << std::endl;      
        return out;      
      }      
      ORMObjectMember Feed::members_[3] = {      
        { "last_updated", &(null<Feed>()->last_updated), 0},        
        { "url", &(null<Feed>()->url), 0},        
        { "feed_id", &(null<Feed>()->feed_id_), 0},        
      };
      
      Feed::Feed (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Feed::Feed (DB::IConn& connection,      
        const ORMInt::value_type& feed_id        
      )      
        :ORMObject<postgres_connection>(connection), feed_id_(feed_id)
      {}
      
      Feed::Feed(const Feed& from)      
        :ORMObject<postgres_connection>(from), feed_id_(from.feed_id_),
        last_updated(from.last_updated),      
        url(from.url)      
      {      
      }
      
      Feed& Feed::operator=(const Feed& from)      
      {      
        Unused(from);      
        feed_id_ = from.feed_id_;      
        last_updated = from.last_updated;      
        url = from.url;      
        return *this;      
      }
      
      bool Feed::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "url "      
          "FROM feed "      
          "WHERE feed_id = :i1"));      
        query  << feed_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Feed::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE feed SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE feed_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Feed::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (feed_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('feed_feed_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> feed_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO feed (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " feed_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Feed::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM feed WHERE feed_id = :i1"));      
        query  << feed_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Feed::del ()      
      {      
        return delet();      
      }
      
      void Feed::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Feed" << std::endl;      
          out << "{" << std::endl;      
          out << "* feed_id = " << strof(feed_id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Feed because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Feed& val)      
      {      
        out << "Feed:" << std::endl;      
        out << " *feed_id: " << val.feed_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  url: " << val.url << std::endl;      
        return out;      
      }      
      ORMObjectMember Feedstate::members_[3] = {      
        { "items", &(null<Feedstate>()->items), 0},        
        { "last_update", &(null<Feedstate>()->last_update), 0},        
        { "feed_id", &(null<Feedstate>()->feed_id_), 0},        
      };
      
      Feedstate::Feedstate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Feedstate::Feedstate (DB::IConn& connection,      
        const ORMInt::value_type& feed_id        
      )      
        :ORMObject<postgres_connection>(connection), feed_id_(feed_id)
      {}
      
      Feedstate::Feedstate(const Feedstate& from)      
        :ORMObject<postgres_connection>(from), feed_id_(from.feed_id_),
        items(from.items),      
        last_update(from.last_update)      
      {      
      }
      
      Feedstate& Feedstate::operator=(const Feedstate& from)      
      {      
        Unused(from);      
        items = from.items;      
        last_update = from.last_update;      
        return *this;      
      }
      
      bool Feedstate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "items,"      
            "last_update "      
          "FROM feedstate "      
          "WHERE feed_id = :i1"));      
        query  << feed_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Feedstate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE feedstate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE feed_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Feedstate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (feed_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(feed_id) + 1 FROM feedstate"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> feed_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO feedstate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " feed_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Feedstate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM feedstate WHERE feed_id = :i1"));      
        query  << feed_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Feedstate::del ()      
      {      
        return delet();      
      }
      
      void Feedstate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Feedstate" << std::endl;      
          out << "{" << std::endl;      
          out << "* feed_id = " << strof(feed_id_) << ";" << std::endl;      
          out << "  items = " << strof(items) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Feedstate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Feedstate& val)      
      {      
        out << "Feedstate:" << std::endl;      
        out << " *feed_id: " << val.feed_id_ << std::endl;      
        out << "  items: " << val.items << std::endl;      
        out << "  last_update: " << val.last_update << std::endl;      
        return out;      
      }      
      ORMObjectMember FreqCap::members_[7] = {      
        { "last_updated", &(null<FreqCap>()->last_updated), 0},        
        { "life_count", &(null<FreqCap>()->life_count), 0},        
        { "period", &(null<FreqCap>()->period), 0},        
        { "version", &(null<FreqCap>()->version), 0},        
        { "window_count", &(null<FreqCap>()->window_count), 0},        
        { "window_length", &(null<FreqCap>()->window_length), 0},        
        { "freq_cap_id", &(null<FreqCap>()->id_), 0},        
      };
      
      FreqCap::FreqCap (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      FreqCap::FreqCap (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      FreqCap::FreqCap(const FreqCap& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        last_updated(from.last_updated),      
        life_count(from.life_count),      
        period(from.period),      
        version(from.version),      
        window_count(from.window_count),      
        window_length(from.window_length)      
      {      
      }
      
      FreqCap& FreqCap::operator=(const FreqCap& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        last_updated = from.last_updated;      
        life_count = from.life_count;      
        period = from.period;      
        version = from.version;      
        window_count = from.window_count;      
        window_length = from.window_length;      
        return *this;      
      }
      
      bool FreqCap::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE freqcap SET VERSION = now()";      
        {      
          strm << " WHERE freq_cap_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool FreqCap::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated,"      
            "life_count,"      
            "period,"      
            "version,"      
            "window_count,"      
            "window_length "      
          "FROM freqcap "      
          "WHERE freq_cap_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool FreqCap::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE freqcap SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 6; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE freq_cap_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool FreqCap::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('freqcap_freq_cap_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO freqcap (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 6; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " freq_cap_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool FreqCap::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM freqcap WHERE freq_cap_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool FreqCap::del ()      
      {      
        return delet();      
      }
      
      void FreqCap::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: FreqCap" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  life_count = " << strof(life_count) << ";" << std::endl;      
          out << "  period = " << strof(period) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "  window_count = " << strof(window_count) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log FreqCap because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const FreqCap& val)      
      {      
        out << "FreqCap:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  life_count: " << val.life_count << std::endl;      
        out << "  period: " << val.period << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  window_count: " << val.window_count << std::endl;      
        out << "  window_length: " << val.window_length << std::endl;      
        return out;      
      }      
      ORMObjectMember SearchEngine::members_[9] = {      
        { "decoding_depth", &(null<SearchEngine>()->decoding_depth), 0},        
        { "encoding", &(null<SearchEngine>()->encoding), 0},        
        { "host", &(null<SearchEngine>()->host), 0},        
        { "last_updated", &(null<SearchEngine>()->last_updated), 0},        
        { "name", &(null<SearchEngine>()->name), 0},        
        { "post_encoding", &(null<SearchEngine>()->post_encoding), 0},        
        { "regexp", &(null<SearchEngine>()->regexp), 0},        
        { "version", &(null<SearchEngine>()->version), 0},        
        { "search_engine_id", &(null<SearchEngine>()->search_engine_id_), 0},        
      };
      
      SearchEngine::SearchEngine (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      SearchEngine::SearchEngine (DB::IConn& connection,      
        const ORMInt::value_type& search_engine_id        
      )      
        :ORMObject<postgres_connection>(connection), search_engine_id_(search_engine_id)
      {}
      
      SearchEngine::SearchEngine(const SearchEngine& from)      
        :ORMObject<postgres_connection>(from), search_engine_id_(from.search_engine_id_),
        decoding_depth(from.decoding_depth),      
        encoding(from.encoding),      
        host(from.host),      
        last_updated(from.last_updated),      
        name(from.name),      
        post_encoding(from.post_encoding),      
        regexp(from.regexp),      
        version(from.version)      
      {      
      }
      
      SearchEngine& SearchEngine::operator=(const SearchEngine& from)      
      {      
        Unused(from);      
        search_engine_id_ = from.search_engine_id_;      
        decoding_depth = from.decoding_depth;      
        encoding = from.encoding;      
        host = from.host;      
        last_updated = from.last_updated;      
        name = from.name;      
        post_encoding = from.post_encoding;      
        regexp = from.regexp;      
        version = from.version;      
        return *this;      
      }
      
      bool SearchEngine::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE searchengine SET VERSION = now()";      
        {      
          strm << " WHERE search_engine_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << search_engine_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SearchEngine::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "decoding_depth,"      
            "encoding,"      
            "host,"      
            "last_updated,"      
            "name,"      
            "post_encoding,"      
            "regexp,"      
            "version "      
          "FROM searchengine "      
          "WHERE search_engine_id = :i1"));      
        query  << search_engine_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool SearchEngine::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE searchengine SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE search_engine_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << search_engine_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SearchEngine::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (search_engine_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('searchengine_search_engine_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> search_engine_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO searchengine (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " search_engine_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << search_engine_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SearchEngine::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM searchengine WHERE search_engine_id = :i1"));      
        query  << search_engine_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool SearchEngine::del ()      
      {      
        return delet();      
      }
      
      void SearchEngine::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: SearchEngine" << std::endl;      
          out << "{" << std::endl;      
          out << "* search_engine_id = " << strof(search_engine_id_) << ";" << std::endl;      
          out << "  decoding_depth = " << strof(decoding_depth) << ";" << std::endl;      
          out << "  encoding = " << strof(encoding) << ";" << std::endl;      
          out << "  host = " << strof(host) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  post_encoding = " << strof(post_encoding) << ";" << std::endl;      
          out << "  regexp = " << strof(regexp) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log SearchEngine because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const SearchEngine& val)      
      {      
        out << "SearchEngine:" << std::endl;      
        out << " *search_engine_id: " << val.search_engine_id_ << std::endl;      
        out << "  decoding_depth: " << val.decoding_depth << std::endl;      
        out << "  encoding: " << val.encoding << std::endl;      
        out << "  host: " << val.host << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  post_encoding: " << val.post_encoding << std::endl;      
        out << "  regexp: " << val.regexp << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Site::members_[17] = {      
        { "account_id", &(null<Site>()->account), 0},        
        { "display_status_id", &(null<Site>()->display_status_id), "1"},        
        { "flags", &(null<Site>()->flags), 0},        
        { "freq_cap_id", &(null<Site>()->freq_cap), 0},        
        { "last_updated", &(null<Site>()->last_updated), 0},        
        { "name", &(null<Site>()->name), 0},        
        { "no_ads_timeout", &(null<Site>()->no_ads_timeout), "0"},        
        { "notes", &(null<Site>()->notes), 0},        
        { "qa_date", &(null<Site>()->qa_date), 0},        
        { "qa_description", &(null<Site>()->qa_description), 0},        
        { "qa_status", &(null<Site>()->qa_status), 0},        
        { "qa_user_id", &(null<Site>()->qa_user_id), 0},        
        { "site_category_id", &(null<Site>()->site_category_id), 0},        
        { "site_url", &(null<Site>()->site_url), 0},        
        { "status", &(null<Site>()->status), 0},        
        { "version", &(null<Site>()->version), 0},        
        { "site_id", &(null<Site>()->id_), 0},        
      };
      
      Site::Site (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Site::Site (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Site::Site(const Site& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        account(from.account),      
        display_status_id(from.display_status_id),      
        flags(from.flags),      
        freq_cap(from.freq_cap),      
        last_updated(from.last_updated),      
        name(from.name),      
        no_ads_timeout(from.no_ads_timeout),      
        notes(from.notes),      
        qa_date(from.qa_date),      
        qa_description(from.qa_description),      
        qa_status(from.qa_status),      
        qa_user_id(from.qa_user_id),      
        site_category_id(from.site_category_id),      
        site_url(from.site_url),      
        status(from.status),      
        version(from.version)      
      {      
      }
      
      Site& Site::operator=(const Site& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        account = from.account;      
        display_status_id = from.display_status_id;      
        flags = from.flags;      
        freq_cap = from.freq_cap;      
        last_updated = from.last_updated;      
        name = from.name;      
        no_ads_timeout = from.no_ads_timeout;      
        notes = from.notes;      
        qa_date = from.qa_date;      
        qa_description = from.qa_description;      
        qa_status = from.qa_status;      
        qa_user_id = from.qa_user_id;      
        site_category_id = from.site_category_id;      
        site_url = from.site_url;      
        status = from.status;      
        version = from.version;      
        return *this;      
      }
      
      bool Site::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE site SET VERSION = now()";      
        {      
          strm << " WHERE site_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Site::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "account_id,"      
            "display_status_id,"      
            "flags,"      
            "freq_cap_id,"      
            "last_updated,"      
            "name,"      
            "no_ads_timeout,"      
            "notes,"      
            "qa_date,"      
            "qa_description,"      
            "qa_status,"      
            "qa_user_id,"      
            "site_category_id,"      
            "site_url,"      
            "status,"      
            "version "      
          "FROM site "      
          "WHERE site_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Site::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE site SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 16; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE site_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Site::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('site_site_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO site (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 16; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " site_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 16; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Site::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM site WHERE site_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Site::del ()      
      {      
        int status_id = get_display_status_id(conn, "site", DS_DELETED);      
        DB::Query  query(conn.get_query("UPDATE site SET"      
          "    STATUS = 'D' "      
          "  , DISPLAY_STATUS_ID = :var1 "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE site_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      bool Site::set_display_status (DisplayStatus status)      
      {      
        int status_id = get_display_status_id(conn, "site", status);      
        DB::Query  query(conn.get_query("UPDATE site SET"      
          "  DISPLAY_STATUS_ID = :var1 "      
          "WHERE site_id = :i1"));      
        query.set(status_id);      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        display_status_id = status_id;      
        return ret;      
      }
      
      void Site::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Site" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  account = " << strof(account) << ";" << std::endl;      
          out << "  display_status_id = " << strof(display_status_id) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  freq_cap = " << strof(freq_cap) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  no_ads_timeout = " << strof(no_ads_timeout) << ";" << std::endl;      
          out << "  notes = " << strof(notes) << ";" << std::endl;      
          out << "  qa_date = " << strof(qa_date) << ";" << std::endl;      
          out << "  qa_description = " << strof(qa_description) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  qa_user_id = " << strof(qa_user_id) << ";" << std::endl;      
          out << "  site_category_id = " << strof(site_category_id) << ";" << std::endl;      
          out << "  site_url = " << strof(site_url) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Site because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Site& val)      
      {      
        out << "Site:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  account: " << val.account << std::endl;      
        out << "  display_status_id: " << val.display_status_id << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  freq_cap: " << val.freq_cap << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  no_ads_timeout: " << val.no_ads_timeout << std::endl;      
        out << "  notes: " << val.notes << std::endl;      
        out << "  qa_date: " << val.qa_date << std::endl;      
        out << "  qa_description: " << val.qa_description << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  qa_user_id: " << val.qa_user_id << std::endl;      
        out << "  site_category_id: " << val.site_category_id << std::endl;      
        out << "  site_url: " << val.site_url << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Sitecategory::members_[5] = {      
        { "country_code", &(null<Sitecategory>()->country_code), 0},        
        { "last_updated", &(null<Sitecategory>()->last_updated), 0},        
        { "name", &(null<Sitecategory>()->name), 0},        
        { "version", &(null<Sitecategory>()->version), 0},        
        { "site_category_id", &(null<Sitecategory>()->site_category_id_), 0},        
      };
      
      Sitecategory::Sitecategory (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Sitecategory::Sitecategory (DB::IConn& connection,      
        const ORMInt::value_type& site_category_id        
      )      
        :ORMObject<postgres_connection>(connection), site_category_id_(site_category_id)
      {}
      
      Sitecategory::Sitecategory(const Sitecategory& from)      
        :ORMObject<postgres_connection>(from), site_category_id_(from.site_category_id_),
        country_code(from.country_code),      
        last_updated(from.last_updated),      
        name(from.name),      
        version(from.version)      
      {      
      }
      
      Sitecategory& Sitecategory::operator=(const Sitecategory& from)      
      {      
        Unused(from);      
        site_category_id_ = from.site_category_id_;      
        country_code = from.country_code;      
        last_updated = from.last_updated;      
        name = from.name;      
        version = from.version;      
        return *this;      
      }
      
      bool Sitecategory::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE sitecategory SET VERSION = now()";      
        {      
          strm << " WHERE site_category_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << site_category_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sitecategory::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "country_code,"      
            "last_updated,"      
            "name,"      
            "version "      
          "FROM sitecategory "      
          "WHERE site_category_id = :i1"));      
        query  << site_category_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Sitecategory::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE sitecategory SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE site_category_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << site_category_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sitecategory::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (site_category_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('sitecategory_site_category_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> site_category_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO sitecategory (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " site_category_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << site_category_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sitecategory::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM sitecategory WHERE site_category_id = :i1"));      
        query  << site_category_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Sitecategory::del ()      
      {      
        return delet();      
      }
      
      void Sitecategory::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Sitecategory" << std::endl;      
          out << "{" << std::endl;      
          out << "* site_category_id = " << strof(site_category_id_) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Sitecategory because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Sitecategory& val)      
      {      
        out << "Sitecategory:" << std::endl;      
        out << " *site_category_id: " << val.site_category_id_ << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember SiteCreativeApproval::members_[7] = {      
        { "approval", &(null<SiteCreativeApproval>()->approval), 0},        
        { "approval_date", &(null<SiteCreativeApproval>()->approval_date), 0},        
        { "feedback", &(null<SiteCreativeApproval>()->feedback), 0},        
        { "last_updated", &(null<SiteCreativeApproval>()->last_updated), 0},        
        { "reject_reason_id", &(null<SiteCreativeApproval>()->reject_reason_id), 0},        
        { "creative_id", &(null<SiteCreativeApproval>()->creative_id_), 0},        
        { "site_id", &(null<SiteCreativeApproval>()->site_id_), 0},        
      };
      
      SiteCreativeApproval::SiteCreativeApproval (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      SiteCreativeApproval::SiteCreativeApproval (DB::IConn& connection,      
        const ORMInt::value_type& creative_id,        
        const ORMInt::value_type& site_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_id_(creative_id), site_id_(site_id)
      {}
      
      SiteCreativeApproval::SiteCreativeApproval(const SiteCreativeApproval& from)      
        :ORMObject<postgres_connection>(from), creative_id_(from.creative_id_), site_id_(from.site_id_),
        approval(from.approval),      
        approval_date(from.approval_date),      
        feedback(from.feedback),      
        last_updated(from.last_updated),      
        reject_reason_id(from.reject_reason_id)      
      {      
      }
      
      SiteCreativeApproval& SiteCreativeApproval::operator=(const SiteCreativeApproval& from)      
      {      
        Unused(from);      
        approval = from.approval;      
        approval_date = from.approval_date;      
        feedback = from.feedback;      
        last_updated = from.last_updated;      
        reject_reason_id = from.reject_reason_id;      
        return *this;      
      }
      
      bool SiteCreativeApproval::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "approval,"      
            "approval_date,"      
            "feedback,"      
            "last_updated,"      
            "reject_reason_id "      
          "FROM sitecreativeapproval "      
          "WHERE creative_id = :i1 AND site_id = :i2"));      
        query  << creative_id_ << site_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool SiteCreativeApproval::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE sitecreativeapproval SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_id = :i1 AND site_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteCreativeApproval::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_id) + 1 FROM sitecreativeapproval"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO sitecreativeapproval (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_id, site_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteCreativeApproval::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM sitecreativeapproval WHERE creative_id = :i1 AND site_id = :i2"));      
        query  << creative_id_ << site_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool SiteCreativeApproval::del ()      
      {      
        return delet();      
      }
      
      void SiteCreativeApproval::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: SiteCreativeApproval" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_id = " << strof(creative_id_) << ";" << std::endl;      
          out << "* site_id = " << strof(site_id_) << ";" << std::endl;      
          out << "  approval = " << strof(approval) << ";" << std::endl;      
          out << "  approval_date = " << strof(approval_date) << ";" << std::endl;      
          out << "  feedback = " << strof(feedback) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log SiteCreativeApproval because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const SiteCreativeApproval& val)      
      {      
        out << "SiteCreativeApproval:" << std::endl;      
        out << " *creative_id: " << val.creative_id_ << std::endl;      
        out << " *site_id: " << val.site_id_ << std::endl;      
        out << "  approval: " << val.approval << std::endl;      
        out << "  approval_date: " << val.approval_date << std::endl;      
        out << "  feedback: " << val.feedback << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  reject_reason_id: " << val.reject_reason_id << std::endl;      
        return out;      
      }      
      ORMObjectMember SiteCreativeCategoryExclusion::members_[4] = {      
        { "approval", &(null<SiteCreativeCategoryExclusion>()->approval), 0},        
        { "last_updated", &(null<SiteCreativeCategoryExclusion>()->last_updated), 0},        
        { "creative_category_id", &(null<SiteCreativeCategoryExclusion>()->creative_category_id_), 0},        
        { "site_id", &(null<SiteCreativeCategoryExclusion>()->site_id_), 0},        
      };
      
      SiteCreativeCategoryExclusion::SiteCreativeCategoryExclusion (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      SiteCreativeCategoryExclusion::SiteCreativeCategoryExclusion (DB::IConn& connection,      
        const ORMInt::value_type& creative_category_id,        
        const ORMInt::value_type& site_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_category_id_(creative_category_id), site_id_(site_id)
      {}
      
      SiteCreativeCategoryExclusion::SiteCreativeCategoryExclusion(const SiteCreativeCategoryExclusion& from)      
        :ORMObject<postgres_connection>(from), creative_category_id_(from.creative_category_id_), site_id_(from.site_id_),
        approval(from.approval),      
        last_updated(from.last_updated)      
      {      
      }
      
      SiteCreativeCategoryExclusion& SiteCreativeCategoryExclusion::operator=(const SiteCreativeCategoryExclusion& from)      
      {      
        Unused(from);      
        approval = from.approval;      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool SiteCreativeCategoryExclusion::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "approval,"      
            "last_updated "      
          "FROM sitecreativecategoryexclusion "      
          "WHERE creative_category_id = :i1 AND site_id = :i2"));      
        query  << creative_category_id_ << site_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool SiteCreativeCategoryExclusion::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE sitecreativecategoryexclusion SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_category_id = :i1 AND site_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteCreativeCategoryExclusion::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_category_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_category_id) + 1 FROM sitecreativecategoryexclusion"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_category_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO sitecreativecategoryexclusion (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_category_id, site_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << site_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteCreativeCategoryExclusion::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM sitecreativecategoryexclusion WHERE creative_category_id = :i1 AND site_id = :i2"));      
        query  << creative_category_id_ << site_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool SiteCreativeCategoryExclusion::del ()      
      {      
        return delet();      
      }
      
      void SiteCreativeCategoryExclusion::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: SiteCreativeCategoryExclusion" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_category_id = " << strof(creative_category_id_) << ";" << std::endl;      
          out << "* site_id = " << strof(site_id_) << ";" << std::endl;      
          out << "  approval = " << strof(approval) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log SiteCreativeCategoryExclusion because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const SiteCreativeCategoryExclusion& val)      
      {      
        out << "SiteCreativeCategoryExclusion:" << std::endl;      
        out << " *creative_category_id: " << val.creative_category_id_ << std::endl;      
        out << " *site_id: " << val.site_id_ << std::endl;      
        out << "  approval: " << val.approval << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember SiteRate::members_[6] = {      
        { "effective_date", &(null<SiteRate>()->effective_date), "now()"},        
        { "last_updated", &(null<SiteRate>()->last_updated), 0},        
        { "rate", &(null<SiteRate>()->rate), 0},        
        { "rate_type", &(null<SiteRate>()->rate_type), 0},        
        { "tag_pricing_id", &(null<SiteRate>()->tag_pricing), 0},        
        { "site_rate_id", &(null<SiteRate>()->id_), 0},        
      };
      
      SiteRate::SiteRate (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      SiteRate::SiteRate (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      SiteRate::SiteRate(const SiteRate& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        effective_date(from.effective_date),      
        last_updated(from.last_updated),      
        rate(from.rate),      
        rate_type(from.rate_type),      
        tag_pricing(from.tag_pricing)      
      {      
      }
      
      SiteRate& SiteRate::operator=(const SiteRate& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        effective_date = from.effective_date;      
        last_updated = from.last_updated;      
        rate = from.rate;      
        rate_type = from.rate_type;      
        tag_pricing = from.tag_pricing;      
        return *this;      
      }
      
      bool SiteRate::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "effective_date,"      
            "last_updated,"      
            "rate,"      
            "rate_type,"      
            "tag_pricing_id "      
          "FROM siterate "      
          "WHERE site_rate_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool SiteRate::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE siterate SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE site_rate_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteRate::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('siterate_site_rate_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO siterate (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " site_rate_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool SiteRate::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM siterate WHERE site_rate_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool SiteRate::del ()      
      {      
        return delet();      
      }
      
      void SiteRate::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: SiteRate" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  effective_date = " << strof(effective_date) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  rate = " << strof(rate) << ";" << std::endl;      
          out << "  rate_type = " << strof(rate_type) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log SiteRate because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const SiteRate& val)      
      {      
        out << "SiteRate:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  effective_date: " << val.effective_date << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  rate: " << val.rate << std::endl;      
        out << "  rate_type: " << val.rate_type << std::endl;      
        out << "  tag_pricing: " << val.tag_pricing << std::endl;      
        return out;      
      }      
      ORMObjectMember Sizetype::members_[10] = {      
        { "flags", &(null<Sizetype>()->flags), 0},        
        { "last_updated", &(null<Sizetype>()->last_updated), 0},        
        { "name", &(null<Sizetype>()->name), 0},        
        { "tag_templ_brpb_file", &(null<Sizetype>()->tag_templ_brpb_file), 0},        
        { "tag_templ_iest_file", &(null<Sizetype>()->tag_templ_iest_file), 0},        
        { "tag_templ_iframe_file", &(null<Sizetype>()->tag_templ_iframe_file), 0},        
        { "tag_templ_preview_file", &(null<Sizetype>()->tag_templ_preview_file), 0},        
        { "tag_template_file", &(null<Sizetype>()->tag_template_file), 0},        
        { "version", &(null<Sizetype>()->version), 0},        
        { "size_type_id", &(null<Sizetype>()->size_type_id_), 0},        
      };
      
      Sizetype::Sizetype (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Sizetype::Sizetype (DB::IConn& connection,      
        const ORMInt::value_type& size_type_id        
      )      
        :ORMObject<postgres_connection>(connection), size_type_id_(size_type_id)
      {}
      
      Sizetype::Sizetype(const Sizetype& from)      
        :ORMObject<postgres_connection>(from), size_type_id_(from.size_type_id_),
        flags(from.flags),      
        last_updated(from.last_updated),      
        name(from.name),      
        tag_templ_brpb_file(from.tag_templ_brpb_file),      
        tag_templ_iest_file(from.tag_templ_iest_file),      
        tag_templ_iframe_file(from.tag_templ_iframe_file),      
        tag_templ_preview_file(from.tag_templ_preview_file),      
        tag_template_file(from.tag_template_file),      
        version(from.version)      
      {      
      }
      
      Sizetype& Sizetype::operator=(const Sizetype& from)      
      {      
        Unused(from);      
        size_type_id_ = from.size_type_id_;      
        flags = from.flags;      
        last_updated = from.last_updated;      
        name = from.name;      
        tag_templ_brpb_file = from.tag_templ_brpb_file;      
        tag_templ_iest_file = from.tag_templ_iest_file;      
        tag_templ_iframe_file = from.tag_templ_iframe_file;      
        tag_templ_preview_file = from.tag_templ_preview_file;      
        tag_template_file = from.tag_template_file;      
        version = from.version;      
        return *this;      
      }
      
      bool Sizetype::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE sizetype SET VERSION = now()";      
        {      
          strm << " WHERE size_type_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << size_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sizetype::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "flags,"      
            "last_updated,"      
            "name,"      
            "tag_templ_brpb_file,"      
            "tag_templ_iest_file,"      
            "tag_templ_iframe_file,"      
            "tag_templ_preview_file,"      
            "tag_template_file,"      
            "version "      
          "FROM sizetype "      
          "WHERE size_type_id = :i1"));      
        query  << size_type_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Sizetype::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE sizetype SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE size_type_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << size_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sizetype::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (size_type_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('sizetype_size_type_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> size_type_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO sizetype (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 9; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " size_type_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 9; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << size_type_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Sizetype::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM sizetype WHERE size_type_id = :i1"));      
        query  << size_type_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Sizetype::del ()      
      {      
        return delet();      
      }
      
      void Sizetype::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Sizetype" << std::endl;      
          out << "{" << std::endl;      
          out << "* size_type_id = " << strof(size_type_id_) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  tag_templ_brpb_file = " << strof(tag_templ_brpb_file) << ";" << std::endl;      
          out << "  tag_templ_iest_file = " << strof(tag_templ_iest_file) << ";" << std::endl;      
          out << "  tag_templ_iframe_file = " << strof(tag_templ_iframe_file) << ";" << std::endl;      
          out << "  tag_templ_preview_file = " << strof(tag_templ_preview_file) << ";" << std::endl;      
          out << "  tag_template_file = " << strof(tag_template_file) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Sizetype because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Sizetype& val)      
      {      
        out << "Sizetype:" << std::endl;      
        out << " *size_type_id: " << val.size_type_id_ << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  tag_templ_brpb_file: " << val.tag_templ_brpb_file << std::endl;      
        out << "  tag_templ_iest_file: " << val.tag_templ_iest_file << std::endl;      
        out << "  tag_templ_iframe_file: " << val.tag_templ_iframe_file << std::endl;      
        out << "  tag_templ_preview_file: " << val.tag_templ_preview_file << std::endl;      
        out << "  tag_template_file: " << val.tag_template_file << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember TagPricing::members_[9] = {      
        { "ccg_rate_type", &(null<TagPricing>()->ccg_rate_type), 0},        
        { "ccg_type", &(null<TagPricing>()->ccg_type), 0},        
        { "country_code", &(null<TagPricing>()->country_code), 0},        
        { "last_updated", &(null<TagPricing>()->last_updated), 0},        
        { "site_rate_id", &(null<TagPricing>()->site_rate), 0},        
        { "status", &(null<TagPricing>()->status), 0},        
        { "tag_id", &(null<TagPricing>()->tag), 0},        
        { "version", &(null<TagPricing>()->version), 0},        
        { "tag_pricing_id", &(null<TagPricing>()->id_), 0},        
      };
      
      TagPricing::TagPricing (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      TagPricing::TagPricing (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      TagPricing::TagPricing(const TagPricing& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        ccg_rate_type(from.ccg_rate_type),      
        ccg_type(from.ccg_type),      
        country_code(from.country_code),      
        last_updated(from.last_updated),      
        site_rate(from.site_rate),      
        status(from.status),      
        tag(from.tag),      
        version(from.version)      
      {      
      }
      
      TagPricing& TagPricing::operator=(const TagPricing& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        ccg_rate_type = from.ccg_rate_type;      
        ccg_type = from.ccg_type;      
        country_code = from.country_code;      
        last_updated = from.last_updated;      
        site_rate = from.site_rate;      
        status = from.status;      
        tag = from.tag;      
        version = from.version;      
        return *this;      
      }
      
      bool TagPricing::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE tagpricing SET VERSION = now()";      
        {      
          strm << " WHERE tag_pricing_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool TagPricing::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "ccg_rate_type,"      
            "ccg_type,"      
            "country_code,"      
            "last_updated,"      
            "site_rate_id,"      
            "status,"      
            "tag_id,"      
            "version "      
          "FROM tagpricing "      
          "WHERE tag_pricing_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool TagPricing::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE tagpricing SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE tag_pricing_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool TagPricing::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('tagpricing_tag_pricing_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO tagpricing (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " tag_pricing_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool TagPricing::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM tagpricing WHERE tag_pricing_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool TagPricing::del ()      
      {      
        return delet();      
      }
      
      void TagPricing::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: TagPricing" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  ccg_rate_type = " << strof(ccg_rate_type) << ";" << std::endl;      
          out << "  ccg_type = " << strof(ccg_type) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  site_rate = " << strof(site_rate) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  tag = " << strof(tag) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log TagPricing because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const TagPricing& val)      
      {      
        out << "TagPricing:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  ccg_rate_type: " << val.ccg_rate_type << std::endl;      
        out << "  ccg_type: " << val.ccg_type << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  site_rate: " << val.site_rate << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  tag: " << val.tag << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Tags::members_[13] = {      
        { "allow_expandable", &(null<Tags>()->allow_expandable), "'N'"},        
        { "flags", &(null<Tags>()->flags), "0"},        
        { "last_updated", &(null<Tags>()->last_updated), 0},        
        { "marketplace", &(null<Tags>()->marketplace), "'WG'"},        
        { "name", &(null<Tags>()->name), 0},        
        { "passback", &(null<Tags>()->passback), 0},        
        { "passback_code", &(null<Tags>()->passback_code), 0},        
        { "passback_type", &(null<Tags>()->passback_type), "'HTML_URL'"},        
        { "site_id", &(null<Tags>()->site), 0},        
        { "size_type_id", &(null<Tags>()->size_type_id), 0},        
        { "status", &(null<Tags>()->status), "'A'"},        
        { "version", &(null<Tags>()->version), 0},        
        { "tag_id", &(null<Tags>()->id_), 0},        
      };
      
      Tags::Tags (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Tags::Tags (DB::IConn& connection,      
        const ORMInt::value_type& id        
      )      
        :ORMObject<postgres_connection>(connection), id_(id)
      {}
      
      Tags::Tags(const Tags& from)      
        :ORMObject<postgres_connection>(from), id_(from.id_),
        allow_expandable(from.allow_expandable),      
        flags(from.flags),      
        last_updated(from.last_updated),      
        marketplace(from.marketplace),      
        name(from.name),      
        passback(from.passback),      
        passback_code(from.passback_code),      
        passback_type(from.passback_type),      
        site(from.site),      
        size_type_id(from.size_type_id),      
        status(from.status),      
        version(from.version)      
      {      
      }
      
      Tags& Tags::operator=(const Tags& from)      
      {      
        Unused(from);      
        id_ = from.id_;      
        allow_expandable = from.allow_expandable;      
        flags = from.flags;      
        last_updated = from.last_updated;      
        marketplace = from.marketplace;      
        name = from.name;      
        passback = from.passback;      
        passback_code = from.passback_code;      
        passback_type = from.passback_type;      
        site = from.site;      
        size_type_id = from.size_type_id;      
        status = from.status;      
        version = from.version;      
        return *this;      
      }
      
      bool Tags::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE tags SET VERSION = now()";      
        {      
          strm << " WHERE tag_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tags::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "allow_expandable,"      
            "flags,"      
            "last_updated,"      
            "marketplace,"      
            "name,"      
            "passback,"      
            "passback_code,"      
            "passback_type,"      
            "site_id,"      
            "size_type_id,"      
            "status,"      
            "version "      
          "FROM tags "      
          "WHERE tag_id = :i1"));      
        query  << id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 12; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Tags::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE tags SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 12; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE tag_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 12; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tags::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('tags_tag_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO tags (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 12; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " tag_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 12; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 12; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tags::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM tags WHERE tag_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Tags::del ()      
      {      
        DB::Query  query(conn.get_query("UPDATE tags SET"      
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE tag_id = :i1"));      
        query  << id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      void Tags::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Tags" << std::endl;      
          out << "{" << std::endl;      
          out << "* id = " << strof(id_) << ";" << std::endl;      
          out << "  allow_expandable = " << strof(allow_expandable) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  marketplace = " << strof(marketplace) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  passback = " << strof(passback) << ";" << std::endl;      
          out << "  passback_code = " << strof(passback_code) << ";" << std::endl;      
          out << "  passback_type = " << strof(passback_type) << ";" << std::endl;      
          out << "  site = " << strof(site) << ";" << std::endl;      
          out << "  size_type_id = " << strof(size_type_id) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Tags because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Tags& val)      
      {      
        out << "Tags:" << std::endl;      
        out << " *id: " << val.id_ << std::endl;      
        out << "  allow_expandable: " << val.allow_expandable << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  marketplace: " << val.marketplace << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  passback: " << val.passback << std::endl;      
        out << "  passback_code: " << val.passback_code << std::endl;      
        out << "  passback_type: " << val.passback_type << std::endl;      
        out << "  site: " << val.site << std::endl;      
        out << "  size_type_id: " << val.size_type_id << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Tagscreativecategoryexclusion::members_[4] = {      
        { "approval", &(null<Tagscreativecategoryexclusion>()->approval), 0},        
        { "last_updated", &(null<Tagscreativecategoryexclusion>()->last_updated), 0},        
        { "creative_category_id", &(null<Tagscreativecategoryexclusion>()->creative_category_id_), 0},        
        { "tag_id", &(null<Tagscreativecategoryexclusion>()->tag_id_), 0},        
      };
      
      Tagscreativecategoryexclusion::Tagscreativecategoryexclusion (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Tagscreativecategoryexclusion::Tagscreativecategoryexclusion (DB::IConn& connection,      
        const ORMInt::value_type& creative_category_id,        
        const ORMInt::value_type& tag_id        
      )      
        :ORMObject<postgres_connection>(connection), creative_category_id_(creative_category_id), tag_id_(tag_id)
      {}
      
      Tagscreativecategoryexclusion::Tagscreativecategoryexclusion(const Tagscreativecategoryexclusion& from)      
        :ORMObject<postgres_connection>(from), creative_category_id_(from.creative_category_id_), tag_id_(from.tag_id_),
        approval(from.approval),      
        last_updated(from.last_updated)      
      {      
      }
      
      Tagscreativecategoryexclusion& Tagscreativecategoryexclusion::operator=(const Tagscreativecategoryexclusion& from)      
      {      
        Unused(from);      
        approval = from.approval;      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool Tagscreativecategoryexclusion::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "approval,"      
            "last_updated "      
          "FROM tagscreativecategoryexclusion "      
          "WHERE creative_category_id = :i1 AND tag_id = :i2"));      
        query  << creative_category_id_ << tag_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Tagscreativecategoryexclusion::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE tagscreativecategoryexclusion SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE creative_category_id = :i1 AND tag_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << tag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tagscreativecategoryexclusion::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (creative_category_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(creative_category_id) + 1 FROM tagscreativecategoryexclusion"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> creative_category_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO tagscreativecategoryexclusion (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 2; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " creative_category_id, tag_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 2; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << creative_category_id_ << tag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tagscreativecategoryexclusion::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM tagscreativecategoryexclusion WHERE creative_category_id = :i1 AND tag_id = :i2"));      
        query  << creative_category_id_ << tag_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Tagscreativecategoryexclusion::del ()      
      {      
        return delet();      
      }
      
      void Tagscreativecategoryexclusion::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Tagscreativecategoryexclusion" << std::endl;      
          out << "{" << std::endl;      
          out << "* creative_category_id = " << strof(creative_category_id_) << ";" << std::endl;      
          out << "* tag_id = " << strof(tag_id_) << ";" << std::endl;      
          out << "  approval = " << strof(approval) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Tagscreativecategoryexclusion because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Tagscreativecategoryexclusion& val)      
      {      
        out << "Tagscreativecategoryexclusion:" << std::endl;      
        out << " *creative_category_id: " << val.creative_category_id_ << std::endl;      
        out << " *tag_id: " << val.tag_id_ << std::endl;      
        out << "  approval: " << val.approval << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember Tag_tagsize::members_[3] = {      
        { "last_updated", &(null<Tag_tagsize>()->last_updated), 0},        
        { "size_id", &(null<Tag_tagsize>()->size_id_), 0},        
        { "tag_id", &(null<Tag_tagsize>()->tag_id_), 0},        
      };
      
      Tag_tagsize::Tag_tagsize (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Tag_tagsize::Tag_tagsize (DB::IConn& connection,      
        const ORMInt::value_type& size_id,        
        const ORMInt::value_type& tag_id        
      )      
        :ORMObject<postgres_connection>(connection), size_id_(size_id), tag_id_(tag_id)
      {}
      
      Tag_tagsize::Tag_tagsize(const Tag_tagsize& from)      
        :ORMObject<postgres_connection>(from), size_id_(from.size_id_), tag_id_(from.tag_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      Tag_tagsize& Tag_tagsize::operator=(const Tag_tagsize& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool Tag_tagsize::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM tag_tagsize "      
          "WHERE size_id = :i1 AND tag_id = :i2"));      
        query  << size_id_ << tag_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Tag_tagsize::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE tag_tagsize SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE size_id = :i1 AND tag_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << size_id_ << tag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tag_tagsize::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (size_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(size_id) + 1 FROM tag_tagsize"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> size_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO tag_tagsize (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " size_id, tag_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << size_id_ << tag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Tag_tagsize::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM tag_tagsize WHERE size_id = :i1 AND tag_id = :i2"));      
        query  << size_id_ << tag_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Tag_tagsize::del ()      
      {      
        return delet();      
      }
      
      void Tag_tagsize::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Tag_tagsize" << std::endl;      
          out << "{" << std::endl;      
          out << "* size_id = " << strof(size_id_) << ";" << std::endl;      
          out << "* tag_id = " << strof(tag_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Tag_tagsize because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Tag_tagsize& val)      
      {      
        out << "Tag_tagsize:" << std::endl;      
        out << " *size_id: " << val.size_id_ << std::endl;      
        out << " *tag_id: " << val.tag_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember Template::members_[7] = {      
        { "expandable", &(null<Template>()->expandable), "'N'"},        
        { "last_updated", &(null<Template>()->last_updated), 0},        
        { "name", &(null<Template>()->name), 0},        
        { "status", &(null<Template>()->status), 0},        
        { "template_type", &(null<Template>()->template_type), "'CREATIVE'"},        
        { "version", &(null<Template>()->version), 0},        
        { "template_id", &(null<Template>()->template_id_), 0},        
      };
      
      Template::Template (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Template::Template (DB::IConn& connection,      
        const ORMInt::value_type& template_id        
      )      
        :ORMObject<postgres_connection>(connection), template_id_(template_id)
      {}
      
      Template::Template(const Template& from)      
        :ORMObject<postgres_connection>(from), template_id_(from.template_id_),
        expandable(from.expandable),      
        last_updated(from.last_updated),      
        name(from.name),      
        status(from.status),      
        template_type(from.template_type),      
        version(from.version)      
      {      
      }
      
      Template& Template::operator=(const Template& from)      
      {      
        Unused(from);      
        template_id_ = from.template_id_;      
        expandable = from.expandable;      
        last_updated = from.last_updated;      
        name = from.name;      
        status = from.status;      
        template_type = from.template_type;      
        version = from.version;      
        return *this;      
      }
      
      bool Template::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE template SET VERSION = now()";      
        {      
          strm << " WHERE template_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << template_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Template::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "expandable,"      
            "last_updated,"      
            "name,"      
            "status,"      
            "template_type,"      
            "version "      
          "FROM template "      
          "WHERE template_id = :i1"));      
        query  << template_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Template::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE template SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 6; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE template_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << template_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Template::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (template_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('template_template_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> template_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO template (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 6; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " template_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 6; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << template_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Template::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM template WHERE template_id = :i1"));      
        query  << template_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Template::del ()      
      {      
        DB::Query  query(conn.get_query("UPDATE template SET"      
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE template_id = :i1"));      
        query  << template_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      void Template::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Template" << std::endl;      
          out << "{" << std::endl;      
          out << "* template_id = " << strof(template_id_) << ";" << std::endl;      
          out << "  expandable = " << strof(expandable) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  template_type = " << strof(template_type) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Template because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Template& val)      
      {      
        out << "Template:" << std::endl;      
        out << " *template_id: " << val.template_id_ << std::endl;      
        out << "  expandable: " << val.expandable << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  template_type: " << val.template_type << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Templatefile::members_[9] = {      
        { "app_format_id", &(null<Templatefile>()->app_format_id), 0},        
        { "flags", &(null<Templatefile>()->flags), 0},        
        { "last_updated", &(null<Templatefile>()->last_updated), 0},        
        { "size_id", &(null<Templatefile>()->size_id), 0},        
        { "template_file", &(null<Templatefile>()->template_file), 0},        
        { "template_id", &(null<Templatefile>()->template_id), 0},        
        { "template_type", &(null<Templatefile>()->template_type), 0},        
        { "version", &(null<Templatefile>()->version), 0},        
        { "template_file_id", &(null<Templatefile>()->template_file_id_), 0},        
      };
      
      Templatefile::Templatefile (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Templatefile::Templatefile (DB::IConn& connection,      
        const ORMInt::value_type& template_file_id        
      )      
        :ORMObject<postgres_connection>(connection), template_file_id_(template_file_id)
      {}
      
      Templatefile::Templatefile(const Templatefile& from)      
        :ORMObject<postgres_connection>(from), template_file_id_(from.template_file_id_),
        app_format_id(from.app_format_id),      
        flags(from.flags),      
        last_updated(from.last_updated),      
        size_id(from.size_id),      
        template_file(from.template_file),      
        template_id(from.template_id),      
        template_type(from.template_type),      
        version(from.version)      
      {      
      }
      
      Templatefile& Templatefile::operator=(const Templatefile& from)      
      {      
        Unused(from);      
        template_file_id_ = from.template_file_id_;      
        app_format_id = from.app_format_id;      
        flags = from.flags;      
        last_updated = from.last_updated;      
        size_id = from.size_id;      
        template_file = from.template_file;      
        template_id = from.template_id;      
        template_type = from.template_type;      
        version = from.version;      
        return *this;      
      }
      
      bool Templatefile::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE templatefile SET VERSION = now()";      
        {      
          strm << " WHERE template_file_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << template_file_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Templatefile::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "app_format_id,"      
            "flags,"      
            "last_updated,"      
            "size_id,"      
            "template_file,"      
            "template_id,"      
            "template_type,"      
            "version "      
          "FROM templatefile "      
          "WHERE template_file_id = :i1"));      
        query  << template_file_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Templatefile::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE templatefile SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE template_file_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << template_file_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Templatefile::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (template_file_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('templatefile_template_file_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> template_file_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO templatefile (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " template_file_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << template_file_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Templatefile::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM templatefile WHERE template_file_id = :i1"));      
        query  << template_file_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Templatefile::del ()      
      {      
        return delet();      
      }
      
      void Templatefile::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Templatefile" << std::endl;      
          out << "{" << std::endl;      
          out << "* template_file_id = " << strof(template_file_id_) << ";" << std::endl;      
          out << "  app_format_id = " << strof(app_format_id) << ";" << std::endl;      
          out << "  flags = " << strof(flags) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  size_id = " << strof(size_id) << ";" << std::endl;      
          out << "  template_file = " << strof(template_file) << ";" << std::endl;      
          out << "  template_id = " << strof(template_id) << ";" << std::endl;      
          out << "  template_type = " << strof(template_type) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Templatefile because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Templatefile& val)      
      {      
        out << "Templatefile:" << std::endl;      
        out << " *template_file_id: " << val.template_file_id_ << std::endl;      
        out << "  app_format_id: " << val.app_format_id << std::endl;      
        out << "  flags: " << val.flags << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  size_id: " << val.size_id << std::endl;      
        out << "  template_file: " << val.template_file << std::endl;      
        out << "  template_id: " << val.template_id << std::endl;      
        out << "  template_type: " << val.template_type << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Triggers::members_[9] = {      
        { "channel_type", &(null<Triggers>()->channel_type), 0},        
        { "country_code", &(null<Triggers>()->country_code), 0},        
        { "created", &(null<Triggers>()->created), "timestamp 'now'"},        
        { "last_updated", &(null<Triggers>()->last_updated), 0},        
        { "normalized_trigger", &(null<Triggers>()->normalized_trigger), 0},        
        { "qa_status", &(null<Triggers>()->qa_status), 0},        
        { "trigger_type", &(null<Triggers>()->trigger_type), 0},        
        { "version", &(null<Triggers>()->version), "timestamp 'now'"},        
        { "trigger_id", &(null<Triggers>()->trigger_id_), 0},        
      };
      
      Triggers::Triggers (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Triggers::Triggers (DB::IConn& connection,      
        const ORMInt::value_type& trigger_id        
      )      
        :ORMObject<postgres_connection>(connection), trigger_id_(trigger_id)
      {}
      
      Triggers::Triggers(const Triggers& from)      
        :ORMObject<postgres_connection>(from), trigger_id_(from.trigger_id_),
        channel_type(from.channel_type),      
        country_code(from.country_code),      
        created(from.created),      
        last_updated(from.last_updated),      
        normalized_trigger(from.normalized_trigger),      
        qa_status(from.qa_status),      
        trigger_type(from.trigger_type),      
        version(from.version)      
      {      
      }
      
      Triggers& Triggers::operator=(const Triggers& from)      
      {      
        Unused(from);      
        trigger_id_ = from.trigger_id_;      
        channel_type = from.channel_type;      
        country_code = from.country_code;      
        created = from.created;      
        last_updated = from.last_updated;      
        normalized_trigger = from.normalized_trigger;      
        qa_status = from.qa_status;      
        trigger_type = from.trigger_type;      
        version = from.version;      
        return *this;      
      }
      
      bool Triggers::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE triggers SET VERSION = now()";      
        {      
          strm << " WHERE trigger_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << trigger_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Triggers::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "channel_type,"      
            "country_code,"      
            "created,"      
            "last_updated,"      
            "normalized_trigger,"      
            "qa_status,"      
            "trigger_type,"      
            "version "      
          "FROM triggers "      
          "WHERE trigger_id = :i1"));      
        query  << trigger_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Triggers::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE triggers SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE trigger_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << trigger_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Triggers::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (trigger_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('triggers_trigger_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> trigger_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO triggers (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 8; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " trigger_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 8; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << trigger_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Triggers::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM triggers WHERE trigger_id = :i1"));      
        query  << trigger_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Triggers::del ()      
      {      
        return delet();      
      }
      
      void Triggers::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Triggers" << std::endl;      
          out << "{" << std::endl;      
          out << "* trigger_id = " << strof(trigger_id_) << ";" << std::endl;      
          out << "  channel_type = " << strof(channel_type) << ";" << std::endl;      
          out << "  country_code = " << strof(country_code) << ";" << std::endl;      
          out << "  created = " << strof(created) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  normalized_trigger = " << strof(normalized_trigger) << ";" << std::endl;      
          out << "  qa_status = " << strof(qa_status) << ";" << std::endl;      
          out << "  trigger_type = " << strof(trigger_type) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Triggers because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Triggers& val)      
      {      
        out << "Triggers:" << std::endl;      
        out << " *trigger_id: " << val.trigger_id_ << std::endl;      
        out << "  channel_type: " << val.channel_type << std::endl;      
        out << "  country_code: " << val.country_code << std::endl;      
        out << "  created: " << val.created << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  normalized_trigger: " << val.normalized_trigger << std::endl;      
        out << "  qa_status: " << val.qa_status << std::endl;      
        out << "  trigger_type: " << val.trigger_type << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember Wdrequestmapping::members_[6] = {      
        { "description", &(null<Wdrequestmapping>()->description), 0},        
        { "last_updated", &(null<Wdrequestmapping>()->last_updated), 0},        
        { "name", &(null<Wdrequestmapping>()->name), 0},        
        { "request", &(null<Wdrequestmapping>()->request), 0},        
        { "version", &(null<Wdrequestmapping>()->version), 0},        
        { "wd_req_mapping_id", &(null<Wdrequestmapping>()->wd_req_mapping_id_), 0},        
      };
      
      Wdrequestmapping::Wdrequestmapping (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      Wdrequestmapping::Wdrequestmapping (DB::IConn& connection,      
        const ORMInt::value_type& wd_req_mapping_id        
      )      
        :ORMObject<postgres_connection>(connection), wd_req_mapping_id_(wd_req_mapping_id)
      {}
      
      Wdrequestmapping::Wdrequestmapping(const Wdrequestmapping& from)      
        :ORMObject<postgres_connection>(from), wd_req_mapping_id_(from.wd_req_mapping_id_),
        description(from.description),      
        last_updated(from.last_updated),      
        name(from.name),      
        request(from.request),      
        version(from.version)      
      {      
      }
      
      Wdrequestmapping& Wdrequestmapping::operator=(const Wdrequestmapping& from)      
      {      
        Unused(from);      
        wd_req_mapping_id_ = from.wd_req_mapping_id_;      
        description = from.description;      
        last_updated = from.last_updated;      
        name = from.name;      
        request = from.request;      
        version = from.version;      
        return *this;      
      }
      
      bool Wdrequestmapping::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE wdrequestmapping SET VERSION = now()";      
        {      
          strm << " WHERE wd_req_mapping_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << wd_req_mapping_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Wdrequestmapping::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "description,"      
            "last_updated,"      
            "name,"      
            "request,"      
            "version "      
          "FROM wdrequestmapping "      
          "WHERE wd_req_mapping_id = :i1"));      
        query  << wd_req_mapping_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool Wdrequestmapping::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE wdrequestmapping SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE wd_req_mapping_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << wd_req_mapping_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Wdrequestmapping::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (wd_req_mapping_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('wdrequestmapping_wd_req_mapping_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> wd_req_mapping_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO wdrequestmapping (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 5; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " wd_req_mapping_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 5; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << wd_req_mapping_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool Wdrequestmapping::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM wdrequestmapping WHERE wd_req_mapping_id = :i1"));      
        query  << wd_req_mapping_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool Wdrequestmapping::del ()      
      {      
        return delet();      
      }
      
      void Wdrequestmapping::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: Wdrequestmapping" << std::endl;      
          out << "{" << std::endl;      
          out << "* wd_req_mapping_id = " << strof(wd_req_mapping_id_) << ";" << std::endl;      
          out << "  description = " << strof(description) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  request = " << strof(request) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log Wdrequestmapping because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const Wdrequestmapping& val)      
      {      
        out << "Wdrequestmapping:" << std::endl;      
        out << " *wd_req_mapping_id: " << val.wd_req_mapping_id_ << std::endl;      
        out << "  description: " << val.description << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  request: " << val.request << std::endl;      
        out << "  version: " << val.version << std::endl;      
        return out;      
      }      
      ORMObjectMember WDTag::members_[12] = {      
        { "height", &(null<WDTag>()->height), 0},        
        { "last_updated", &(null<WDTag>()->last_updated), 0},        
        { "name", &(null<WDTag>()->name), 0},        
        { "opted_in_content", &(null<WDTag>()->opted_in_content), 0},        
        { "opted_out_content", &(null<WDTag>()->opted_out_content), 0},        
        { "passback", &(null<WDTag>()->passback), 0},        
        { "site_id", &(null<WDTag>()->site_id), 0},        
        { "status", &(null<WDTag>()->status), 0},        
        { "template_id", &(null<WDTag>()->template_id), 0},        
        { "version", &(null<WDTag>()->version), "timestamp 'now'"},        
        { "width", &(null<WDTag>()->width), 0},        
        { "wdtag_id", &(null<WDTag>()->wdtag_id_), 0},        
      };
      
      WDTag::WDTag (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      WDTag::WDTag (DB::IConn& connection,      
        const ORMInt::value_type& wdtag_id        
      )      
        :ORMObject<postgres_connection>(connection), wdtag_id_(wdtag_id)
      {}
      
      WDTag::WDTag(const WDTag& from)      
        :ORMObject<postgres_connection>(from), wdtag_id_(from.wdtag_id_),
        height(from.height),      
        last_updated(from.last_updated),      
        name(from.name),      
        opted_in_content(from.opted_in_content),      
        opted_out_content(from.opted_out_content),      
        passback(from.passback),      
        site_id(from.site_id),      
        status(from.status),      
        template_id(from.template_id),      
        version(from.version),      
        width(from.width)      
      {      
      }
      
      WDTag& WDTag::operator=(const WDTag& from)      
      {      
        Unused(from);      
        wdtag_id_ = from.wdtag_id_;      
        height = from.height;      
        last_updated = from.last_updated;      
        name = from.name;      
        opted_in_content = from.opted_in_content;      
        opted_out_content = from.opted_out_content;      
        passback = from.passback;      
        site_id = from.site_id;      
        status = from.status;      
        template_id = from.template_id;      
        version = from.version;      
        width = from.width;      
        return *this;      
      }
      
      bool WDTag::touch ()      
      {      
        std::ostringstream strm;      
        strm << "UPDATE wdtag SET VERSION = now()";      
        {      
          strm << " WHERE wdtag_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          query  << wdtag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTag::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "height,"      
            "last_updated,"      
            "name,"      
            "opted_in_content,"      
            "opted_out_content,"      
            "passback,"      
            "site_id,"      
            "status,"      
            "template_id,"      
            "version,"      
            "width "      
          "FROM wdtag "      
          "WHERE wdtag_id = :i1"));      
        query  << wdtag_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool WDTag::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE wdtag SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE wdtag_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTag::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (wdtag_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT  nextval('wdtag_wdtag_id_seq')"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> wdtag_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO wdtag (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 11; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " wdtag_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 11; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTag::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM wdtag WHERE wdtag_id = :i1"));      
        query  << wdtag_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool WDTag::del ()      
      {      
        DB::Query  query(conn.get_query("UPDATE wdtag SET"      
          "    STATUS = 'D' "      
          "  , NAME = concat(NAME, CONCAT('-D-', TO_CHAR(now(), 'YYMMDDhhmmss'))) "      
          "WHERE wdtag_id = :i1"));      
        query  << wdtag_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        this->select();      
        return ret;      
      }
      
      void WDTag::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: WDTag" << std::endl;      
          out << "{" << std::endl;      
          out << "* wdtag_id = " << strof(wdtag_id_) << ";" << std::endl;      
          out << "  height = " << strof(height) << ";" << std::endl;      
          out << "  last_updated = " << strof(last_updated) << ";" << std::endl;      
          out << "  name = " << strof(name) << ";" << std::endl;      
          out << "  opted_in_content = " << strof(opted_in_content) << ";" << std::endl;      
          out << "  opted_out_content = " << strof(opted_out_content) << ";" << std::endl;      
          out << "  passback = " << strof(passback) << ";" << std::endl;      
          out << "  site_id = " << strof(site_id) << ";" << std::endl;      
          out << "  status = " << strof(status) << ";" << std::endl;      
          out << "  template_id = " << strof(template_id) << ";" << std::endl;      
          out << "  version = " << strof(version) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log WDTag because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const WDTag& val)      
      {      
        out << "WDTag:" << std::endl;      
        out << " *wdtag_id: " << val.wdtag_id_ << std::endl;      
        out << "  height: " << val.height << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        out << "  name: " << val.name << std::endl;      
        out << "  opted_in_content: " << val.opted_in_content << std::endl;      
        out << "  opted_out_content: " << val.opted_out_content << std::endl;      
        out << "  passback: " << val.passback << std::endl;      
        out << "  site_id: " << val.site_id << std::endl;      
        out << "  status: " << val.status << std::endl;      
        out << "  template_id: " << val.template_id << std::endl;      
        out << "  version: " << val.version << std::endl;      
        out << "  width: " << val.width << std::endl;      
        return out;      
      }      
      ORMObjectMember WDTagfeed_optedin::members_[3] = {      
        { "last_updated", &(null<WDTagfeed_optedin>()->last_updated), 0},        
        { "wdtag_id", &(null<WDTagfeed_optedin>()->wdtag_id_), 0},        
        { "feed_id", &(null<WDTagfeed_optedin>()->feed_id_), 0},        
      };
      
      WDTagfeed_optedin::WDTagfeed_optedin (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      WDTagfeed_optedin::WDTagfeed_optedin (DB::IConn& connection,      
        const ORMInt::value_type& wdtag_id,        
        const ORMInt::value_type& feed_id        
      )      
        :ORMObject<postgres_connection>(connection), wdtag_id_(wdtag_id), feed_id_(feed_id)
      {}
      
      WDTagfeed_optedin::WDTagfeed_optedin(const WDTagfeed_optedin& from)      
        :ORMObject<postgres_connection>(from), wdtag_id_(from.wdtag_id_), feed_id_(from.feed_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      WDTagfeed_optedin& WDTagfeed_optedin::operator=(const WDTagfeed_optedin& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool WDTagfeed_optedin::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM wdtagfeed_optedin "      
          "WHERE wdtag_id = :i1 AND feed_id = :i2"));      
        query  << wdtag_id_ << feed_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedin::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE wdtagfeed_optedin SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE wdtag_id = :i1 AND feed_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_ << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedin::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (wdtag_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(wdtag_id) + 1 FROM wdtagfeed_optedin"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> wdtag_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO wdtagfeed_optedin (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " wdtag_id, feed_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_ << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedin::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM wdtagfeed_optedin WHERE wdtag_id = :i1 AND feed_id = :i2"));      
        query  << wdtag_id_ << feed_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool WDTagfeed_optedin::del ()      
      {      
        return delet();      
      }
      
      void WDTagfeed_optedin::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: WDTagfeed_optedin" << std::endl;      
          out << "{" << std::endl;      
          out << "* wdtag_id = " << strof(wdtag_id_) << ";" << std::endl;      
          out << "* feed_id = " << strof(feed_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log WDTagfeed_optedin because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedin& val)      
      {      
        out << "WDTagfeed_optedin:" << std::endl;      
        out << " *wdtag_id: " << val.wdtag_id_ << std::endl;      
        out << " *feed_id: " << val.feed_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember WDTagfeed_optedout::members_[3] = {      
        { "last_updated", &(null<WDTagfeed_optedout>()->last_updated), 0},        
        { "wdtag_id", &(null<WDTagfeed_optedout>()->wdtag_id_), 0},        
        { "feed_id", &(null<WDTagfeed_optedout>()->feed_id_), 0},        
      };
      
      WDTagfeed_optedout::WDTagfeed_optedout (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      WDTagfeed_optedout::WDTagfeed_optedout (DB::IConn& connection,      
        const ORMInt::value_type& wdtag_id,        
        const ORMInt::value_type& feed_id        
      )      
        :ORMObject<postgres_connection>(connection), wdtag_id_(wdtag_id), feed_id_(feed_id)
      {}
      
      WDTagfeed_optedout::WDTagfeed_optedout(const WDTagfeed_optedout& from)      
        :ORMObject<postgres_connection>(from), wdtag_id_(from.wdtag_id_), feed_id_(from.feed_id_),
        last_updated(from.last_updated)      
      {      
      }
      
      WDTagfeed_optedout& WDTagfeed_optedout::operator=(const WDTagfeed_optedout& from)      
      {      
        Unused(from);      
        last_updated = from.last_updated;      
        return *this;      
      }
      
      bool WDTagfeed_optedout::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "last_updated "      
          "FROM wdtagfeed_optedout "      
          "WHERE wdtag_id = :i1 AND feed_id = :i2"));      
        query  << wdtag_id_ << feed_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedout::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE wdtagfeed_optedout SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE wdtag_id = :i1 AND feed_id = :i2";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_ << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedout::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (wdtag_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(wdtag_id) + 1 FROM wdtagfeed_optedout"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> wdtag_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO wdtagfeed_optedout (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 1; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " wdtag_id, feed_id )";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1, :i2)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 1; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << wdtag_id_ << feed_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WDTagfeed_optedout::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM wdtagfeed_optedout WHERE wdtag_id = :i1 AND feed_id = :i2"));      
        query  << wdtag_id_ << feed_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool WDTagfeed_optedout::del ()      
      {      
        return delet();      
      }
      
      void WDTagfeed_optedout::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: WDTagfeed_optedout" << std::endl;      
          out << "{" << std::endl;      
          out << "* wdtag_id = " << strof(wdtag_id_) << ";" << std::endl;      
          out << "* feed_id = " << strof(feed_id_) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log WDTagfeed_optedout because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedout& val)      
      {      
        out << "WDTagfeed_optedout:" << std::endl;      
        out << " *wdtag_id: " << val.wdtag_id_ << std::endl;      
        out << " *feed_id: " << val.feed_id_ << std::endl;      
        out << "  last_updated: " << val.last_updated << std::endl;      
        return out;      
      }      
      ORMObjectMember WebwiseDiscoverItem::members_[5] = {      
        { "language", &(null<WebwiseDiscoverItem>()->language), 0},        
        { "link", &(null<WebwiseDiscoverItem>()->link), 0},        
        { "pub_date", &(null<WebwiseDiscoverItem>()->pub_date), 0},        
        { "title", &(null<WebwiseDiscoverItem>()->title), 0},        
        { "item_id", &(null<WebwiseDiscoverItem>()->item_id_), 0},        
      };
      
      WebwiseDiscoverItem::WebwiseDiscoverItem (DB::IConn& connection)      
        :ORMObject<postgres_connection>(connection)
      {}
      
      WebwiseDiscoverItem::WebwiseDiscoverItem (DB::IConn& connection,      
        const ORMString::value_type& item_id        
      )      
        :ORMObject<postgres_connection>(connection), item_id_(item_id)
      {}
      
      WebwiseDiscoverItem::WebwiseDiscoverItem(const WebwiseDiscoverItem& from)      
        :ORMObject<postgres_connection>(from), item_id_(from.item_id_),
        language(from.language),      
        link(from.link),      
        pub_date(from.pub_date),      
        title(from.title)      
      {      
      }
      
      WebwiseDiscoverItem& WebwiseDiscoverItem::operator=(const WebwiseDiscoverItem& from)      
      {      
        Unused(from);      
        language = from.language;      
        link = from.link;      
        pub_date = from.pub_date;      
        title = from.title;      
        return *this;      
      }
      
      bool WebwiseDiscoverItem::select ()      
      {      
        DB::Query query(conn.get_query("SELECT "      
            "language,"      
            "link,"      
            "pub_date,"      
            "title "      
          "FROM webwisediscoveritem "      
          "WHERE item_id = :i1"));      
        query  << item_id_;      
        DB::Result result(query.ask());      
        if(result.next())      
        {      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            result >> members_[i].value(this);      
          }      
          return true;      
        }      
        return false;      
      }
      
      bool WebwiseDiscoverItem::update (bool set_defaults)      
      {      
        Unused(set_defaults);      
        std::ostringstream strm;      
        strm << "UPDATE webwisediscoveritem SET ";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = update_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter > 1)      
        {      
          strm << " WHERE item_id = :i1";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            setin_(query, members_[i].value(this));      
          }      
          query  << item_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WebwiseDiscoverItem::insert (bool set_defaults)      
      {      
        Unused(set_defaults);      
        if (item_id_.is_null())      
        {        
          DB::Query  query(conn.get_query("SELECT Max(item_id) + 1 FROM webwisediscoveritem"));        
          DB::Result result(query.ask());        
          if(!result.next())        
            return false;        
          result >> item_id_;        
        }        
        std::ostringstream strm;      
        strm << "INSERT INTO webwisediscoveritem (";      
        int counter = 1;      
        for(unsigned int i = 0; i < 4; ++i)      
        {      
          counter = insert_(strm, counter, this, members_[i], set_defaults);      
        }      
        if(counter >= 1)      
        {      
          strm << ((counter > 1)? ",": " ") << " item_id)";      
          strm <<" VALUES (";      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            put_var_ (strm, i,  this, members_[i], set_defaults);      
          }      
          strm << ":i1)";      
          DB::Query  query(conn.get_query(strm.str()));      
          for(unsigned int i = 0; i < 4; ++i)      
          {      
            insertin_(query, members_[i].value(this));      
          }      
          query  << item_id_;      
          bool ret = query.update() > 0;      
          conn.commit();      
          return ret;      
        }      
        return false;      
      }
      
      bool WebwiseDiscoverItem::delet ()      
      {      
        DB::Query  query(conn.get_query("DELETE FROM webwisediscoveritem WHERE item_id = :i1"));      
        query  << item_id_;      
        bool ret = query.update() > 0;      
        conn.commit();      
        return ret;      
      }
      
      bool WebwiseDiscoverItem::del ()      
      {      
        return delet();      
      }
      
      void WebwiseDiscoverItem::log_in (Logger& log, unsigned long severity)      
      {      
        if(select())      
        {      
          std::ostringstream out;      
          out << "Data record: WebwiseDiscoverItem" << std::endl;      
          out << "{" << std::endl;      
          out << "* item_id = " << strof(item_id_) << ";" << std::endl;      
          out << "  language = " << strof(language) << ";" << std::endl;      
          out << "  link = " << strof(link) << ";" << std::endl;      
          out << "  pub_date = " << strof(pub_date) << ";" << std::endl;      
          out << "}" << std::endl;      
          log.log(out.str(), severity);      
        }      
        else      
        {      
          throw Exception("error to log WebwiseDiscoverItem because not select");      
        }      
      }
      
      std::ostream& operator<< (std::ostream& out, const WebwiseDiscoverItem& val)      
      {      
        out << "WebwiseDiscoverItem:" << std::endl;      
        out << " *item_id: " << val.item_id_ << std::endl;      
        out << "  language: " << val.language << std::endl;      
        out << "  link: " << val.link << std::endl;      
        out << "  pub_date: " << val.pub_date << std::endl;      
        out << "  title: " << val.title << std::endl;      
        return out;      
      }      
    }
  }
}
