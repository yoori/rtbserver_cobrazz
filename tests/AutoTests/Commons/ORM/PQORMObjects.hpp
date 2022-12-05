
#ifndef _AUTOTEST_COMMONS_ORM_PQORMOBJECTS_HPP
#define _AUTOTEST_COMMONS_ORM_PQORMOBJECTS_HPP
 
#include "ORM.hpp"
 
namespace AutoTest
{
  namespace ORM
  {
    namespace PQ
    {
      namespace DB = AutoTest::DBC;

      class Account;      
      class Accountaddress;      
      class Accountfinancialdata;      
      class Accountrole;      
      class Accounttype;      
      class Action;      
      class Appformat;      
      class BehavioralParameters;      
      class Behavioralparameterslist;      
      class Campaign;      
      class CampaignCreative;      
      class CampaignCreativeGroup;      
      class Ccgaction;      
      class CCGKeyword;      
      class CCGRate;      
      class CCGSite;      
      class Channel;      
      class ChannelInventory;      
      class Channelrate;      
      class Channeltrigger;      
      class Colocation;      
      class ColocationRate;      
      class Colostats;      
      class Country;      
      class Creative;      
      class CreativeCategory;      
      class Creativecategorytype;      
      class CreativeCategory_Creative;      
      class CreativeOptionValue;      
      class Creativesize;      
      class Creative_tagsize;      
      class Currency;      
      class CurrencyExchange;      
      class Currencyexchangerate;      
      class Feed;      
      class Feedstate;      
      class FreqCap;      
      class SearchEngine;      
      class Site;      
      class Sitecategory;      
      class SiteCreativeApproval;      
      class SiteCreativeCategoryExclusion;      
      class SiteRate;      
      class Sizetype;      
      class TagPricing;      
      class Tags;      
      class Tagscreativecategoryexclusion;      
      class Tag_tagsize;      
      class Template;      
      class Templatefile;      
      class Triggers;      
      class Wdrequestmapping;      
      class WDTag;      
      class WDTagfeed_optedin;      
      class WDTagfeed_optedout;      
      class WebwiseDiscoverItem;      

      class Account:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[35];      

        ORMInt         account_id_;        

      public:      

        ORMInt         account_manager_id;        
        ORMInt         account_type_id;        
        Accounttype account_type_id_object() const;        
        ORMInt         adv_contact_id;        
        ORMInt         agency_account_id;        
        ORMInt         billing_address_id;        
        ORMString      business_area;        
        ORMInt         cmp_contact_id;        
        ORMString      company_registration_number;        
        ORMString      contact_name;        
        ORMString      country_code;        
        Country country_code_object() const;        
        ORMInt         currency;        
        Currency currency_object() const;        
        ORMInt         display_status_id;        
        ORMInt         flags;        
        ORMBool        hid_profile;        
        ORMInt         internal_account_id;        
        ORMInt         isp_contact_id;        
        ORMTimestamp   last_deactivated;        
        ORMTimestamp   last_updated;        
        ORMInt         legal_address_id;        
        ORMString      legal_name;        
        ORMFloat       message_sent;        
        ORMString      name;        
        ORMString      notes;        
        ORMBool        passback_below_fold;        
        ORMInt         pub_contact_id;        
        ORMString      pub_pixel_optin;        
        ORMString      pub_pixel_optout;        
        ORMInt         role_id;        
        ORMString      specific_business_area;        
        ORMString      status;        
        ORMString      text_adserving;        
        ORMInt         timezone_id;        
        ORMBool        use_pub_pixel;        
        ORMTimestamp   version;        

        const ORMInt::value_type& account_id () const { return *account_id_;}        

        Account (DB::IConn& connection);      
        Account (const Account& from);      
        Account& operator=(const Account& from);      

        Account (DB::IConn& connection,        
          const ORMInt::value_type& account_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& account_id, bool set_defaults = true)        
        {        
          account_id_ = account_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Account& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Account& val);
      
      class Accountaddress:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[10];      

        ORMInt         address_id_;        

      public:      

        ORMString      city;        
        ORMTimestamp   last_updated;        
        ORMString      line1;        
        ORMString      line2;        
        ORMString      line3;        
        ORMString      province;        
        ORMString      state;        
        ORMTimestamp   version;        
        ORMString      zip;        

        const ORMInt::value_type& address_id () const { return *address_id_;}        

        Accountaddress (DB::IConn& connection);      
        Accountaddress (const Accountaddress& from);      
        Accountaddress& operator=(const Accountaddress& from);      

        Accountaddress (DB::IConn& connection,        
          const ORMInt::value_type& address_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& address_id)        
        {        
          address_id_ = address_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& address_id, bool set_defaults = true)        
        {        
          address_id_ = address_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& address_id)        
        {        
          address_id_ = address_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& address_id)        
        {        
          address_id_ = address_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Accountaddress& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Accountaddress& val);
      
      class Accountfinancialdata:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[15];      

        ORMInt         account_id_;        

      public:      

        ORMFloat       camp_credit_used;        
        ORMFloat       invoiced_outstanding;        
        ORMFloat       invoiced_received;        
        ORMTimestamp   last_updated;        
        ORMFloat       not_invoiced;        
        ORMFloat       payments_billed;        
        ORMFloat       payments_paid;        
        ORMFloat       payments_unbilled;        
        ORMFloat       prepaid_amount;        
        ORMFloat       total_adv_amount;        
        ORMFloat       total_paid;        
        ORMString      type;        
        ORMFloat       unbilled_schedule_of_works;        
        ORMTimestamp   version;        

        const ORMInt::value_type& account_id () const { return *account_id_;}        

        Accountfinancialdata (DB::IConn& connection);      
        Accountfinancialdata (const Accountfinancialdata& from);      
        Accountfinancialdata& operator=(const Accountfinancialdata& from);      

        Accountfinancialdata (DB::IConn& connection,        
          const ORMInt::value_type& account_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& account_id, bool set_defaults = true)        
        {        
          account_id_ = account_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& account_id, bool set_defaults = true)        
        {        
          account_id_ = account_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& account_id)        
        {        
          account_id_ = account_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Accountfinancialdata& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Accountfinancialdata& val);
      
      class Accountrole:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         account_role_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      name;        

        const ORMInt::value_type& account_role_id () const { return *account_role_id_;}        

        Accountrole (DB::IConn& connection);      
        Accountrole (const Accountrole& from);      
        Accountrole& operator=(const Accountrole& from);      

        Accountrole (DB::IConn& connection,        
          const ORMInt::value_type& account_role_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& account_role_id)        
        {        
          account_role_id_ = account_role_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& account_role_id, bool set_defaults = true)        
        {        
          account_role_id_ = account_role_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& account_role_id, bool set_defaults = true)        
        {        
          account_role_id_ = account_role_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& account_role_id)        
        {        
          account_role_id_ = account_role_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& account_role_id)        
        {        
          account_role_id_ = account_role_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Accountrole& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Accountrole& val);
      
      class Accounttype:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[26];      

        ORMInt         account_type_id_;        

      public:      

        ORMInt         account_role_id;        
        Accountrole account_role_id_object() const;        
        ORMString      adv_exclusion_approval;        
        ORMString      adv_exclusions;        
        ORMString      auction_rate;        
        ORMFloat       campaign_check_1;        
        ORMFloat       campaign_check_2;        
        ORMFloat       campaign_check_3;        
        ORMBool        campaign_check_on;        
        ORMFloat       channel_check_1;        
        ORMFloat       channel_check_2;        
        ORMFloat       channel_check_3;        
        ORMBool        channel_check_on;        
        ORMInt         flags;        
        ORMBool        io_management;        
        ORMTimestamp   last_updated;        
        ORMFloat       max_keyword_length;        
        ORMFloat       max_keywords_per_channel;        
        ORMFloat       max_keywords_per_group;        
        ORMFloat       max_url_length;        
        ORMFloat       max_urls_per_channel;        
        ORMBool        mobile_operator_targeting;        
        ORMString      name;        
        ORMBool        show_browser_passback_tag;        
        ORMBool        show_iframe_tag;        
        ORMTimestamp   version;        

        const ORMInt::value_type& account_type_id () const { return *account_type_id_;}        

        Accounttype (DB::IConn& connection);      
        Accounttype (const Accounttype& from);      
        Accounttype& operator=(const Accounttype& from);      

        Accounttype (DB::IConn& connection,        
          const ORMInt::value_type& account_type_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& account_type_id)        
        {        
          account_type_id_ = account_type_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& account_type_id, bool set_defaults = true)        
        {        
          account_type_id_ = account_type_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& account_type_id)        
        {        
          account_type_id_ = account_type_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& account_type_id)        
        {        
          account_type_id_ = account_type_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Accounttype& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Accounttype& val);
      
      class Action:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[12];      

        ORMInt         action_id_;        

      public:      

        ORMInt         account_id;        
        Account account_id_object() const;        
        ORMFloat       click_window;        
        ORMInt         conv_category_id;        
        ORMFloat       cur_value;        
        ORMInt         display_status_id;        
        ORMFloat       imp_window;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      status;        
        ORMString      url;        
        ORMTimestamp   version;        

        const ORMInt::value_type& action_id () const { return *action_id_;}        

        Action (DB::IConn& connection);      
        Action (const Action& from);      
        Action& operator=(const Action& from);      

        Action (DB::IConn& connection,        
          const ORMInt::value_type& action_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& action_id)        
        {        
          action_id_ = action_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& action_id, bool set_defaults = true)        
        {        
          action_id_ = action_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& action_id)        
        {        
          action_id_ = action_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& action_id)        
        {        
          action_id_ = action_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Action& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Action& val);
      
      class Appformat:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         app_format_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      mime_type;        
        ORMString      name;        
        ORMTimestamp   version;        

        const ORMInt::value_type& app_format_id () const { return *app_format_id_;}        

        Appformat (DB::IConn& connection);      
        Appformat (const Appformat& from);      
        Appformat& operator=(const Appformat& from);      

        Appformat (DB::IConn& connection,        
          const ORMInt::value_type& app_format_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& app_format_id)        
        {        
          app_format_id_ = app_format_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& app_format_id, bool set_defaults = true)        
        {        
          app_format_id_ = app_format_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& app_format_id)        
        {        
          app_format_id_ = app_format_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& app_format_id)        
        {        
          app_format_id_ = app_format_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Appformat& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Appformat& val);
      
      class BehavioralParameters:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[10];      

        ORMInt         behav_params_id_;        

      public:      

        ORMInt         behav_params_list_id;        
        Behavioralparameterslist behav_params_list_id_object() const;        
        ORMInt         channel;        
        Channel channel_object() const;        
        ORMTimestamp   last_updated;        
        ORMFloat       minimum_visits;        
        ORMFloat       time_from;        
        ORMFloat       time_to;        
        ORMString      trigger_type;        
        ORMTimestamp   version;        
        ORMFloat       weight;        

        const ORMInt::value_type& behav_params_id () const { return *behav_params_id_;}        

        BehavioralParameters (DB::IConn& connection);      
        BehavioralParameters (const BehavioralParameters& from);      
        BehavioralParameters& operator=(const BehavioralParameters& from);      

        BehavioralParameters (DB::IConn& connection,        
          const ORMInt::value_type& behav_params_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& behav_params_id)        
        {        
          behav_params_id_ = behav_params_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& behav_params_id, bool set_defaults = true)        
        {        
          behav_params_id_ = behav_params_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& behav_params_id)        
        {        
          behav_params_id_ = behav_params_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& behav_params_id)        
        {        
          behav_params_id_ = behav_params_id;          
          return del ();        
        }        

        bool has_channel (const ORMInt::value_type& channel);        
        bool select_channel (const ORMInt::value_type& channel);        
        bool update_channel (const ORMInt::value_type& channel, bool set_defaults = true);        
        bool delet_channel (const ORMInt::value_type& channel);        
        bool del_channel (const ORMInt::value_type& channel);        
        bool insert_channel (const ORMInt::value_type& channel, bool set_defaults = true)        
        {        
          this->channel = channel;          
          return insert(set_defaults);        
        }        
      friend std::ostream& operator<< (std::ostream& out, const BehavioralParameters& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const BehavioralParameters& val);
      
      class Behavioralparameterslist:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMFloat       threshold;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Behavioralparameterslist (DB::IConn& connection);      
        Behavioralparameterslist (const Behavioralparameterslist& from);      
        Behavioralparameterslist& operator=(const Behavioralparameterslist& from);      

        Behavioralparameterslist (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Behavioralparameterslist& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Behavioralparameterslist& val);
      
      class Campaign:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[23];      

        ORMInt         id_;        

      public:      

        ORMInt         account;        
        Account account_object() const;        
        ORMInt         bill_to_user;        
        ORMFloat       budget;        
        ORMFloat       budget_manual;        
        ORMString      campaign_type;        
        ORMFloat       commission;        
        ORMFloat       daily_budget;        
        ORMTimestamp   date_end;        
        ORMTimestamp   date_start;        
        ORMString      delivery_pacing;        
        ORMInt         display_status_id;        
        ORMInt         flags;        
        ORMInt         freq_cap;        
        FreqCap freq_cap_object() const;        
        ORMTimestamp   last_deactivated;        
        ORMTimestamp   last_updated;        
        ORMString      marketplace;        
        ORMFloat       max_pub_share;        
        ORMString      name;        
        ORMInt         sales_manager_id;        
        ORMInt         sold_to_user;        
        ORMString      status;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Campaign (DB::IConn& connection);      
        Campaign (const Campaign& from);      
        Campaign& operator=(const Campaign& from);      

        Campaign (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Campaign& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Campaign& val);
      
      class CampaignCreative:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[11];      

        ORMInt         id_;        

      public:      

        ORMInt         ccg;        
        CampaignCreativeGroup ccg_object() const;        
        ORMInt         creative;        
        Creative creative_object() const;        
        ORMInt         display_status_id;        
        ORMInt         freq_cap;        
        FreqCap freq_cap_object() const;        
        ORMTimestamp   last_deactivated;        
        ORMTimestamp   last_updated;        
        ORMFloat       set_number;        
        ORMString      status;        
        ORMTimestamp   version;        
        ORMFloat       weight;        

        const ORMInt::value_type& id () const { return *id_;}        

        CampaignCreative (DB::IConn& connection);      
        CampaignCreative (const CampaignCreative& from);      
        CampaignCreative& operator=(const CampaignCreative& from);      

        CampaignCreative (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CampaignCreative& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CampaignCreative& val);
      
      class CampaignCreativeGroup:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[43];      

        ORMInt         id_;        

      public:      

        ORMFloat       budget;        
        ORMInt         campaign;        
        Campaign campaign_object() const;        
        ORMInt         ccg_rate;        
        CCGRate ccg_rate_object() const;        
        ORMString      ccg_type;        
        ORMInt         channel;        
        Channel channel_object() const;        
        ORMString      channel_target;        
        ORMFloat       check_interval_num;        
        ORMString      check_notes;        
        ORMInt         check_user_id;        
        ORMString      country_code;        
        Country country_code_object() const;        
        ORMInt         ctr_reset_id;        
        ORMTimestamp   cur_date;        
        ORMFloat       daily_budget;        
        ORMFloat       daily_clicks;        
        ORMFloat       daily_imp;        
        ORMTimestamp   date_end;        
        ORMTimestamp   date_start;        
        ORMString      delivery_pacing;        
        ORMInt         display_status_id;        
        ORMInt         flags;        
        ORMInt         freq_cap;        
        FreqCap freq_cap_object() const;        
        ORMTimestamp   last_check_date;        
        ORMTimestamp   last_deactivated;        
        ORMTimestamp   last_updated;        
        ORMInt         min_uid_age;        
        ORMString      name;        
        ORMTimestamp   next_check_date;        
        ORMString      optin_status_targeting;        
        ORMTimestamp   qa_date;        
        ORMString      qa_description;        
        ORMString      qa_status;        
        ORMInt         qa_user_id;        
        ORMFloat       realized_budget;        
        ORMInt         rotation_criteria;        
        ORMFloat       selected_mobile_operators;        
        ORMString      status;        
        ORMInt         targeting_channel_id;        
        ORMString      tgt_type;        
        ORMFloat       total_reach;        
        ORMInt         user_sample_group_end;        
        ORMInt         user_sample_group_start;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        CampaignCreativeGroup (DB::IConn& connection);      
        CampaignCreativeGroup (const CampaignCreativeGroup& from);      
        CampaignCreativeGroup& operator=(const CampaignCreativeGroup& from);      

        CampaignCreativeGroup (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CampaignCreativeGroup& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CampaignCreativeGroup& val);
      
      class Ccgaction:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         action_id_;        
        ORMInt         ccg_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& action_id () const { return *action_id_;}        
        const ORMInt::value_type& ccg_id () const { return *ccg_id_;}        

        Ccgaction (DB::IConn& connection);      
        Ccgaction (const Ccgaction& from);      
        Ccgaction& operator=(const Ccgaction& from);      

        Ccgaction (DB::IConn& connection,        
          const ORMInt::value_type& action_id,          
          const ORMInt::value_type& ccg_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& action_id,
          const ORMInt::value_type& ccg_id)        
        {        
          action_id_ = action_id;          
          ccg_id_ = ccg_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& action_id,
          const ORMInt::value_type& ccg_id, bool set_defaults = true)        
        {        
          action_id_ = action_id;          
          ccg_id_ = ccg_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& action_id,
          const ORMInt::value_type& ccg_id, bool set_defaults = true)        
        {        
          action_id_ = action_id;          
          ccg_id_ = ccg_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& action_id,
          const ORMInt::value_type& ccg_id)        
        {        
          action_id_ = action_id;          
          ccg_id_ = ccg_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& action_id,
          const ORMInt::value_type& ccg_id)        
        {        
          action_id_ = action_id;          
          ccg_id_ = ccg_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Ccgaction& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Ccgaction& val);
      
      class CCGKeyword:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[10];      

        ORMInt         id_;        

      public:      

        ORMInt         ccg;        
        CampaignCreativeGroup ccg_object() const;        
        ORMInt         channel_id;        
        Channel channel_id_object() const;        
        ORMString      click_url;        
        ORMTimestamp   last_updated;        
        ORMFloat       max_cpc_bid;        
        ORMString      original_keyword;        
        ORMString      status;        
        ORMString      trigger_type;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        CCGKeyword (DB::IConn& connection);      
        CCGKeyword (const CCGKeyword& from);      
        CCGKeyword& operator=(const CCGKeyword& from);      

        CCGKeyword (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CCGKeyword& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CCGKeyword& val);
      
      class CCGRate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[8];      

        ORMInt         id_;        

      public:      

        ORMInt         ccg;        
        CampaignCreativeGroup ccg_object() const;        
        ORMFloat       cpa;        
        ORMFloat       cpc;        
        ORMFloat       cpm;        
        ORMTimestamp   effective_date;        
        ORMTimestamp   last_updated;        
        ORMString      rate_type;        

        const ORMInt::value_type& id () const { return *id_;}        

        CCGRate (DB::IConn& connection);      
        CCGRate (const CCGRate& from);      
        CCGRate& operator=(const CCGRate& from);      

        CCGRate (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CCGRate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CCGRate& val);
      
      class CCGSite:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         ccg_id_;        
        ORMInt         site_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& ccg_id () const { return *ccg_id_;}        
        const ORMInt::value_type& site_id () const { return *site_id_;}        

        CCGSite (DB::IConn& connection);      
        CCGSite (const CCGSite& from);      
        CCGSite& operator=(const CCGSite& from);      

        CCGSite (DB::IConn& connection,        
          const ORMInt::value_type& ccg_id,          
          const ORMInt::value_type& site_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& ccg_id,
          const ORMInt::value_type& site_id)        
        {        
          ccg_id_ = ccg_id;          
          site_id_ = site_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& ccg_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          ccg_id_ = ccg_id;          
          site_id_ = site_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& ccg_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          ccg_id_ = ccg_id;          
          site_id_ = site_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& ccg_id,
          const ORMInt::value_type& site_id)        
        {        
          ccg_id_ = ccg_id;          
          site_id_ = site_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& ccg_id,
          const ORMInt::value_type& site_id)        
        {        
          ccg_id_ = ccg_id;          
          site_id_ = site_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CCGSite& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CCGSite& val);
      
      class Channel:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[51];      

        ORMInt         id_;        

      public:      

        ORMInt         account;        
        Account account_object() const;        
        ORMString      address;        
        ORMString      base_keyword;        
        ORMInt         behav_params_list_id;        
        Behavioralparameterslist behav_params_list_id_object() const;        
        ORMInt         channel_list_id;        
        ORMString      channel_name_macro;        
        ORMInt         channel_rate_id;        
        Channelrate channel_rate_id_object() const;        
        ORMString      type;        
        ORMFloat       check_interval_num;        
        ORMString      check_notes;        
        ORMInt         check_user_id;        
        ORMString      city_list;        
        ORMString      country_code;        
        Country country_code_object() const;        
        ORMTimestamp   created_date;        
        ORMString      description;        
        ORMString      discover_annotation;        
        ORMString      discover_query;        
        ORMInt         display_status_id;        
        ORMFloat       distinct_url_triggers_count;        
        ORMString      expression;        
        ORMFloat       flags;        
        ORMInt         freq_cap_id;        
        FreqCap freq_cap_id_object() const;        
        ORMString      geo_type;        
        ORMString      keyword_trigger_macro;        
        ORMString      language;        
        ORMTimestamp   last_check_date;        
        ORMTimestamp   last_updated;        
        ORMFloat       latitude;        
        ORMFloat       longitude;        
        ORMFloat       message_sent;        
        ORMString      name;        
        ORMString      channel_namespace;        
        ORMString      newsgate_category_name;        
        ORMTimestamp   next_check_date;        
        ORMInt         parent_channel_id;        
        ORMTimestamp   qa_date;        
        ORMString      qa_description;        
        ORMString      qa_status;        
        ORMInt         qa_user_id;        
        ORMInt         radius;        
        ORMString      radius_units;        
        ORMInt         size_id;        
        ORMString      status;        
        ORMTimestamp   status_change_date;        
        ORMInt         superseded_by_channel_id;        
        ORMString      trigger_type;        
        ORMString      triggers_status;        
        ORMTimestamp   triggers_version;        
        ORMTimestamp   version;        
        ORMString      visibility;        

        const ORMInt::value_type& id () const { return *id_;}        

        Channel (DB::IConn& connection);      
        Channel (const Channel& from);      
        Channel& operator=(const Channel& from);      

        Channel (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        

        bool has_name (const ORMString::value_type& name);        
        bool select_name (const ORMString::value_type& name);        
        bool update_name (const ORMString::value_type& name, bool set_defaults = true);        
        bool delet_name (const ORMString::value_type& name);        
        bool del_name (const ORMString::value_type& name);        
        bool insert_name (const ORMString::value_type& name, bool set_defaults = true)        
        {        
          this->name = name;          
          return insert(set_defaults);        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Channel& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Channel& val);
      
      class ChannelInventory:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[11];      

        ORMDate        sdate_;        
        ORMInt         channel_id_;        
        ORMInt         colo_id_;        

      public:      

        ORMFloat       active_user_count;        
        ORMFloat       hits;        
        ORMFloat       hits_kws;        
        ORMFloat       hits_search_kws;        
        ORMFloat       hits_url_kws;        
        ORMFloat       hits_urls;        
        ORMFloat       sum_ecpm;        
        ORMFloat       total_user_count;        

        const ORMDate::value_type& sdate () const { return *sdate_;}        
        const ORMInt::value_type& channel_id () const { return *channel_id_;}        
        const ORMInt::value_type& colo_id () const { return *colo_id_;}        

        ChannelInventory (DB::IConn& connection);      
        ChannelInventory (const ChannelInventory& from);      
        ChannelInventory& operator=(const ChannelInventory& from);      

        ChannelInventory (DB::IConn& connection,        
          const ORMDate::value_type& sdate,          
          const ORMInt::value_type& channel_id,          
          const ORMInt::value_type& colo_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMDate::value_type& sdate,
          const ORMInt::value_type& channel_id,
          const ORMInt::value_type& colo_id)        
        {        
          sdate_ = sdate;          
          channel_id_ = channel_id;          
          colo_id_ = colo_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMDate::value_type& sdate,
          const ORMInt::value_type& channel_id,
          const ORMInt::value_type& colo_id, bool set_defaults = true)        
        {        
          sdate_ = sdate;          
          channel_id_ = channel_id;          
          colo_id_ = colo_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMDate::value_type& sdate,
          const ORMInt::value_type& channel_id,
          const ORMInt::value_type& colo_id, bool set_defaults = true)        
        {        
          sdate_ = sdate;          
          channel_id_ = channel_id;          
          colo_id_ = colo_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMDate::value_type& sdate,
          const ORMInt::value_type& channel_id,
          const ORMInt::value_type& colo_id)        
        {        
          sdate_ = sdate;          
          channel_id_ = channel_id;          
          colo_id_ = colo_id;          
          return delet ();        
        }        
        bool del  (const ORMDate::value_type& sdate,
          const ORMInt::value_type& channel_id,
          const ORMInt::value_type& colo_id)        
        {        
          sdate_ = sdate;          
          channel_id_ = channel_id;          
          colo_id_ = colo_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const ChannelInventory& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const ChannelInventory& val);
      
      class Channelrate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[8];      

        ORMInt         channel_rate_id_;        

      public:      

        ORMInt         channel_id;        
        Channel channel_id_object() const;        
        ORMFloat       cpc;        
        ORMFloat       cpm;        
        ORMInt         currency_id;        
        Currency currency_id_object() const;        
        ORMTimestamp   effective_date;        
        ORMTimestamp   last_updated;        
        ORMString      rate_type;        

        const ORMInt::value_type& channel_rate_id () const { return *channel_rate_id_;}        

        Channelrate (DB::IConn& connection);      
        Channelrate (const Channelrate& from);      
        Channelrate& operator=(const Channelrate& from);      

        Channelrate (DB::IConn& connection,        
          const ORMInt::value_type& channel_rate_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& channel_rate_id)        
        {        
          channel_rate_id_ = channel_rate_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& channel_rate_id, bool set_defaults = true)        
        {        
          channel_rate_id_ = channel_rate_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& channel_rate_id)        
        {        
          channel_rate_id_ = channel_rate_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& channel_rate_id)        
        {        
          channel_rate_id_ = channel_rate_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Channelrate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Channelrate& val);
      
      class Channeltrigger:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[12];      

        ORMInt         channel_trigger_id_;        

      public:      

        ORMInt         channel_id;        
        Channel channel_id_object() const;        
        ORMString      channel_type;        
        ORMString      country_code;        
        ORMTimestamp   last_updated;        
        ORMBool        masked;        
        ORMBool        negative;        
        ORMString      original_trigger;        
        ORMString      qa_status;        
        ORMString      trigger_group;        
        ORMInt         trigger_id;        
        Triggers trigger_id_object() const;        
        ORMString      trigger_type;        

        const ORMInt::value_type& channel_trigger_id () const { return *channel_trigger_id_;}        

        Channeltrigger (DB::IConn& connection);      
        Channeltrigger (const Channeltrigger& from);      
        Channeltrigger& operator=(const Channeltrigger& from);      

        Channeltrigger (DB::IConn& connection,        
          const ORMInt::value_type& channel_trigger_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& channel_trigger_id)        
        {        
          channel_trigger_id_ = channel_trigger_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& channel_trigger_id, bool set_defaults = true)        
        {        
          channel_trigger_id_ = channel_trigger_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& channel_trigger_id)        
        {        
          channel_trigger_id_ = channel_trigger_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& channel_trigger_id)        
        {        
          channel_trigger_id_ = channel_trigger_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Channeltrigger& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Channeltrigger& val);
      
      class Colocation:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[8];      

        ORMInt         id_;        

      public:      

        ORMInt         account;        
        Account account_object() const;        
        ORMInt         colo_rate;        
        ColocationRate colo_rate_object() const;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      optout_serving;        
        ORMString      status;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Colocation (DB::IConn& connection);      
        Colocation (const Colocation& from);      
        Colocation& operator=(const Colocation& from);      

        Colocation (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        

        bool has_name (const ORMString::value_type& name);        
        bool select_name (const ORMString::value_type& name);        
        bool update_name (const ORMString::value_type& name, bool set_defaults = true);        
        bool delet_name (const ORMString::value_type& name);        
        bool del_name (const ORMString::value_type& name);        
        bool insert_name (const ORMString::value_type& name, bool set_defaults = true)        
        {        
          this->name = name;          
          return insert(set_defaults);        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Colocation& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Colocation& val);
      
      class ColocationRate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         id_;        

      public:      

        ORMInt         colo;        
        Colocation colo_object() const;        
        ORMTimestamp   effective_date;        
        ORMTimestamp   last_updated;        
        ORMFloat       revenue_share;        

        const ORMInt::value_type& id () const { return *id_;}        

        ColocationRate (DB::IConn& connection);      
        ColocationRate (const ColocationRate& from);      
        ColocationRate& operator=(const ColocationRate& from);      

        ColocationRate (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const ColocationRate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const ColocationRate& val);
      
      class Colostats:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         colo_id_;        

      public:      

        ORMDate        last_campaign_update;        
        ORMDate        last_channel_update;        
        ORMDate        last_stats_upload;        
        ORMString      software_version;        

        const ORMInt::value_type& colo_id () const { return *colo_id_;}        

        Colostats (DB::IConn& connection);      
        Colostats (const Colostats& from);      
        Colostats& operator=(const Colostats& from);      

        Colostats (DB::IConn& connection,        
          const ORMInt::value_type& colo_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& colo_id)        
        {        
          colo_id_ = colo_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& colo_id, bool set_defaults = true)        
        {        
          colo_id_ = colo_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& colo_id, bool set_defaults = true)        
        {        
          colo_id_ = colo_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& colo_id)        
        {        
          colo_id_ = colo_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& colo_id)        
        {        
          colo_id_ = colo_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Colostats& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Colostats& val);
      
      class Country:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[25];      

        ORMString      country_code_;        

      public:      

        ORMString      ad_footer_url;        
        ORMString      ad_tag_domain;        
        ORMString      adserving_domain;        
        ORMString      conversion_tag_domain;        
        ORMInt         country_id;        
        ORMInt         currency_id;        
        Currency currency_id_object() const;        
        ORMFloat       default_agency_commission;        
        ORMFloat       default_payment_terms;        
        ORMFloat       default_vat_rate;        
        ORMString      discover_domain;        
        ORMFloat       high_channel_threshold;        
        ORMInt         invoice_custom_report_id;        
        ORMString      language;        
        ORMTimestamp   last_updated;        
        ORMFloat       low_channel_threshold;        
        ORMFloat       max_url_trigger_share;        
        ORMFloat       min_tag_visibility;        
        ORMFloat       min_url_trigger_threshold;        
        ORMFloat       sortorder;        
        ORMString      static_domain;        
        ORMInt         timezone_id;        
        ORMBool        vat_enabled;        
        ORMBool        vat_number_input_enabled;        
        ORMTimestamp   version;        

        const ORMString::value_type& country_code () const { return *country_code_;}        

        Country (DB::IConn& connection);      
        Country (const Country& from);      
        Country& operator=(const Country& from);      

        Country (DB::IConn& connection,        
          const ORMString::value_type& country_code          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMString::value_type& country_code)        
        {        
          country_code_ = country_code;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMString::value_type& country_code, bool set_defaults = true)        
        {        
          country_code_ = country_code;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMString::value_type& country_code)        
        {        
          country_code_ = country_code;          
          return delet ();        
        }        
        bool del  (const ORMString::value_type& country_code)        
        {        
          country_code_ = country_code;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Country& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Country& val);
      
      class Creative:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[17];      

        ORMInt         id_;        

      public:      

        ORMInt         account;        
        Account account_object() const;        
        ORMInt         display_status_id;        
        ORMBool        expandable;        
        ORMString      expansion;        
        ORMInt         flags;        
        ORMTimestamp   last_deactivated;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMTimestamp   qa_date;        
        ORMString      qa_description;        
        ORMString      qa_status;        
        ORMInt         qa_user_id;        
        ORMInt         size;        
        Creativesize size_object() const;        
        ORMString      status;        
        ORMInt         template_id;        
        Template template_id_object() const;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Creative (DB::IConn& connection);      
        Creative (const Creative& from);      
        Creative& operator=(const Creative& from);      

        Creative (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Creative& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Creative& val);
      
      class CreativeCategory:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[6];      

        ORMInt         id_;        

      public:      

        ORMInt         type;        
        Creativecategorytype type_object() const;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      qa_status;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        CreativeCategory (DB::IConn& connection);      
        CreativeCategory (const CreativeCategory& from);      
        CreativeCategory& operator=(const CreativeCategory& from);      

        CreativeCategory (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CreativeCategory& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CreativeCategory& val);
      
      class Creativecategorytype:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[4];      

        ORMInt         cct_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMTimestamp   version;        

        const ORMInt::value_type& cct_id () const { return *cct_id_;}        

        Creativecategorytype (DB::IConn& connection);      
        Creativecategorytype (const Creativecategorytype& from);      
        Creativecategorytype& operator=(const Creativecategorytype& from);      

        Creativecategorytype (DB::IConn& connection,        
          const ORMInt::value_type& cct_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& cct_id)        
        {        
          cct_id_ = cct_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& cct_id, bool set_defaults = true)        
        {        
          cct_id_ = cct_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& cct_id, bool set_defaults = true)        
        {        
          cct_id_ = cct_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& cct_id)        
        {        
          cct_id_ = cct_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& cct_id)        
        {        
          cct_id_ = cct_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Creativecategorytype& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Creativecategorytype& val);
      
      class CreativeCategory_Creative:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         creative_category_id_;        
        ORMInt         creative_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& creative_category_id () const { return *creative_category_id_;}        
        const ORMInt::value_type& creative_id () const { return *creative_id_;}        

        CreativeCategory_Creative (DB::IConn& connection);      
        CreativeCategory_Creative (const CreativeCategory_Creative& from);      
        CreativeCategory_Creative& operator=(const CreativeCategory_Creative& from);      

        CreativeCategory_Creative (DB::IConn& connection,        
          const ORMInt::value_type& creative_category_id,          
          const ORMInt::value_type& creative_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& creative_id)        
        {        
          creative_category_id_ = creative_category_id;          
          creative_id_ = creative_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& creative_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          creative_id_ = creative_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& creative_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          creative_id_ = creative_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& creative_id)        
        {        
          creative_category_id_ = creative_category_id;          
          creative_id_ = creative_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& creative_id)        
        {        
          creative_category_id_ = creative_category_id;          
          creative_id_ = creative_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CreativeCategory_Creative& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CreativeCategory_Creative& val);
      
      class CreativeOptionValue:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         creative_id_;        
        ORMInt         option_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      value;        
        ORMTimestamp   version;        

        const ORMInt::value_type& creative_id () const { return *creative_id_;}        
        const ORMInt::value_type& option_id () const { return *option_id_;}        

        CreativeOptionValue (DB::IConn& connection);      
        CreativeOptionValue (const CreativeOptionValue& from);      
        CreativeOptionValue& operator=(const CreativeOptionValue& from);      

        CreativeOptionValue (DB::IConn& connection,        
          const ORMInt::value_type& creative_id,          
          const ORMInt::value_type& option_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& option_id)        
        {        
          creative_id_ = creative_id;          
          option_id_ = option_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& option_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          option_id_ = option_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& option_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          option_id_ = option_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& option_id)        
        {        
          creative_id_ = creative_id;          
          option_id_ = option_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& option_id)        
        {        
          creative_id_ = creative_id;          
          option_id_ = option_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CreativeOptionValue& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CreativeOptionValue& val);
      
      class Creativesize:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[12];      

        ORMInt         size_id_;        

      public:      

        ORMInt         flags;        
        ORMFloat       height;        
        ORMTimestamp   last_updated;        
        ORMFloat       max_height;        
        ORMFloat       max_width;        
        ORMString      name;        
        ORMString      protocol_name;        
        ORMInt         size_type_id;        
        ORMString      status;        
        ORMTimestamp   version;        
        ORMFloat       width;        

        const ORMInt::value_type& size_id () const { return *size_id_;}        

        Creativesize (DB::IConn& connection);      
        Creativesize (const Creativesize& from);      
        Creativesize& operator=(const Creativesize& from);      

        Creativesize (DB::IConn& connection,        
          const ORMInt::value_type& size_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& size_id)        
        {        
          size_id_ = size_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& size_id, bool set_defaults = true)        
        {        
          size_id_ = size_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& size_id)        
        {        
          size_id_ = size_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& size_id)        
        {        
          size_id_ = size_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Creativesize& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Creativesize& val);
      
      class Creative_tagsize:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         creative_id_;        
        ORMInt         size_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& creative_id () const { return *creative_id_;}        
        const ORMInt::value_type& size_id () const { return *size_id_;}        

        Creative_tagsize (DB::IConn& connection);      
        Creative_tagsize (const Creative_tagsize& from);      
        Creative_tagsize& operator=(const Creative_tagsize& from);      

        Creative_tagsize (DB::IConn& connection,        
          const ORMInt::value_type& creative_id,          
          const ORMInt::value_type& size_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& size_id)        
        {        
          creative_id_ = creative_id;          
          size_id_ = size_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& size_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          size_id_ = size_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& size_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          size_id_ = size_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& size_id)        
        {        
          creative_id_ = creative_id;          
          size_id_ = size_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& size_id)        
        {        
          creative_id_ = creative_id;          
          size_id_ = size_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Creative_tagsize& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Creative_tagsize& val);
      
      class Currency:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[6];      

        ORMInt         currency_id_;        

      public:      

        ORMString      currency_code;        
        ORMFloat       fraction_digits;        
        ORMTimestamp   last_updated;        
        ORMString      source;        
        ORMTimestamp   version;        

        const ORMInt::value_type& currency_id () const { return *currency_id_;}        

        Currency (DB::IConn& connection);      
        Currency (const Currency& from);      
        Currency& operator=(const Currency& from);      

        Currency (DB::IConn& connection,        
          const ORMInt::value_type& currency_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& currency_id)        
        {        
          currency_id_ = currency_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& currency_id, bool set_defaults = true)        
        {        
          currency_id_ = currency_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& currency_id)        
        {        
          currency_id_ = currency_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& currency_id)        
        {        
          currency_id_ = currency_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Currency& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Currency& val);
      
      class CurrencyExchange:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         id_;        

      public:      

        ORMTimestamp   effective_date;        
        ORMTimestamp   last_updated;        

        const ORMInt::value_type& id () const { return *id_;}        

        CurrencyExchange (DB::IConn& connection);      
        CurrencyExchange (const CurrencyExchange& from);      
        CurrencyExchange& operator=(const CurrencyExchange& from);      

        CurrencyExchange (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const CurrencyExchange& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const CurrencyExchange& val);
      
      class Currencyexchangerate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         currency_exchange_id_;        
        ORMInt         currency_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMTimestamp   last_updated_date;        
        ORMFloat       rate;        

        const ORMInt::value_type& currency_exchange_id () const { return *currency_exchange_id_;}        
        const ORMInt::value_type& currency_id () const { return *currency_id_;}        

        Currencyexchangerate (DB::IConn& connection);      
        Currencyexchangerate (const Currencyexchangerate& from);      
        Currencyexchangerate& operator=(const Currencyexchangerate& from);      

        Currencyexchangerate (DB::IConn& connection,        
          const ORMInt::value_type& currency_exchange_id,          
          const ORMInt::value_type& currency_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& currency_exchange_id,
          const ORMInt::value_type& currency_id)        
        {        
          currency_exchange_id_ = currency_exchange_id;          
          currency_id_ = currency_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& currency_exchange_id,
          const ORMInt::value_type& currency_id, bool set_defaults = true)        
        {        
          currency_exchange_id_ = currency_exchange_id;          
          currency_id_ = currency_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& currency_exchange_id,
          const ORMInt::value_type& currency_id, bool set_defaults = true)        
        {        
          currency_exchange_id_ = currency_exchange_id;          
          currency_id_ = currency_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& currency_exchange_id,
          const ORMInt::value_type& currency_id)        
        {        
          currency_exchange_id_ = currency_exchange_id;          
          currency_id_ = currency_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& currency_exchange_id,
          const ORMInt::value_type& currency_id)        
        {        
          currency_exchange_id_ = currency_exchange_id;          
          currency_id_ = currency_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Currencyexchangerate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Currencyexchangerate& val);
      
      class Feed:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         feed_id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMString      url;        

        const ORMInt::value_type& feed_id () const { return *feed_id_;}        

        Feed (DB::IConn& connection);      
        Feed (const Feed& from);      
        Feed& operator=(const Feed& from);      

        Feed (DB::IConn& connection,        
          const ORMInt::value_type& feed_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          feed_id_ = feed_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Feed& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Feed& val);
      
      class Feedstate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         feed_id_;        

      public:      

        ORMInt         items;        
        ORMTimestamp   last_update;        

        const ORMInt::value_type& feed_id () const { return *feed_id_;}        

        Feedstate (DB::IConn& connection);      
        Feedstate (const Feedstate& from);      
        Feedstate& operator=(const Feedstate& from);      

        Feedstate (DB::IConn& connection,        
          const ORMInt::value_type& feed_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          feed_id_ = feed_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          feed_id_ = feed_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& feed_id)        
        {        
          feed_id_ = feed_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Feedstate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Feedstate& val);
      
      class FreqCap:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[7];      

        ORMInt         id_;        

      public:      

        ORMTimestamp   last_updated;        
        ORMFloat       life_count;        
        ORMFloat       period;        
        ORMTimestamp   version;        
        ORMFloat       window_count;        
        ORMFloat       window_length;        

        const ORMInt::value_type& id () const { return *id_;}        

        FreqCap (DB::IConn& connection);      
        FreqCap (const FreqCap& from);      
        FreqCap& operator=(const FreqCap& from);      

        FreqCap (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const FreqCap& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const FreqCap& val);
      
      class SearchEngine:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[9];      

        ORMInt         search_engine_id_;        

      public:      

        ORMFloat       decoding_depth;        
        ORMString      encoding;        
        ORMString      host;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      post_encoding;        
        ORMString      regexp;        
        ORMTimestamp   version;        

        const ORMInt::value_type& search_engine_id () const { return *search_engine_id_;}        

        SearchEngine (DB::IConn& connection);      
        SearchEngine (const SearchEngine& from);      
        SearchEngine& operator=(const SearchEngine& from);      

        SearchEngine (DB::IConn& connection,        
          const ORMInt::value_type& search_engine_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& search_engine_id)        
        {        
          search_engine_id_ = search_engine_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& search_engine_id, bool set_defaults = true)        
        {        
          search_engine_id_ = search_engine_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& search_engine_id)        
        {        
          search_engine_id_ = search_engine_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& search_engine_id)        
        {        
          search_engine_id_ = search_engine_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const SearchEngine& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const SearchEngine& val);
      
      class Site:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[17];      

        ORMInt         id_;        

      public:      

        ORMInt         account;        
        Account account_object() const;        
        ORMInt         display_status_id;        
        ORMInt         flags;        
        ORMInt         freq_cap;        
        FreqCap freq_cap_object() const;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMFloat       no_ads_timeout;        
        ORMString      notes;        
        ORMTimestamp   qa_date;        
        ORMString      qa_description;        
        ORMString      qa_status;        
        ORMInt         qa_user_id;        
        ORMInt         site_category_id;        
        Sitecategory site_category_id_object() const;        
        ORMString      site_url;        
        ORMString      status;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Site (DB::IConn& connection);      
        Site (const Site& from);      
        Site& operator=(const Site& from);      

        Site (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        virtual bool set_display_status (DisplayStatus); //!< set display status      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Site& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Site& val);
      
      class Sitecategory:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMInt         site_category_id_;        

      public:      

        ORMString      country_code;        
        Country country_code_object() const;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMTimestamp   version;        

        const ORMInt::value_type& site_category_id () const { return *site_category_id_;}        

        Sitecategory (DB::IConn& connection);      
        Sitecategory (const Sitecategory& from);      
        Sitecategory& operator=(const Sitecategory& from);      

        Sitecategory (DB::IConn& connection,        
          const ORMInt::value_type& site_category_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& site_category_id)        
        {        
          site_category_id_ = site_category_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& site_category_id, bool set_defaults = true)        
        {        
          site_category_id_ = site_category_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& site_category_id)        
        {        
          site_category_id_ = site_category_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& site_category_id)        
        {        
          site_category_id_ = site_category_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Sitecategory& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Sitecategory& val);
      
      class SiteCreativeApproval:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[7];      

        ORMInt         creative_id_;        
        ORMInt         site_id_;        

      public:      

        ORMString      approval;        
        ORMTimestamp   approval_date;        
        ORMString      feedback;        
        ORMTimestamp   last_updated;        
        ORMInt         reject_reason_id;        

        const ORMInt::value_type& creative_id () const { return *creative_id_;}        
        const ORMInt::value_type& site_id () const { return *site_id_;}        

        SiteCreativeApproval (DB::IConn& connection);      
        SiteCreativeApproval (const SiteCreativeApproval& from);      
        SiteCreativeApproval& operator=(const SiteCreativeApproval& from);      

        SiteCreativeApproval (DB::IConn& connection,        
          const ORMInt::value_type& creative_id,          
          const ORMInt::value_type& site_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_id_ = creative_id;          
          site_id_ = site_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          site_id_ = site_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          creative_id_ = creative_id;          
          site_id_ = site_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_id_ = creative_id;          
          site_id_ = site_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_id_ = creative_id;          
          site_id_ = site_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const SiteCreativeApproval& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const SiteCreativeApproval& val);
      
      class SiteCreativeCategoryExclusion:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[4];      

        ORMInt         creative_category_id_;        
        ORMInt         site_id_;        

      public:      

        ORMString      approval;        
        ORMTimestamp   last_updated;        

        const ORMInt::value_type& creative_category_id () const { return *creative_category_id_;}        
        const ORMInt::value_type& site_id () const { return *site_id_;}        

        SiteCreativeCategoryExclusion (DB::IConn& connection);      
        SiteCreativeCategoryExclusion (const SiteCreativeCategoryExclusion& from);      
        SiteCreativeCategoryExclusion& operator=(const SiteCreativeCategoryExclusion& from);      

        SiteCreativeCategoryExclusion (DB::IConn& connection,        
          const ORMInt::value_type& creative_category_id,          
          const ORMInt::value_type& site_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_category_id_ = creative_category_id;          
          site_id_ = site_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          site_id_ = site_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& site_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          site_id_ = site_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_category_id_ = creative_category_id;          
          site_id_ = site_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& site_id)        
        {        
          creative_category_id_ = creative_category_id;          
          site_id_ = site_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const SiteCreativeCategoryExclusion& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const SiteCreativeCategoryExclusion& val);
      
      class SiteRate:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[6];      

        ORMInt         id_;        

      public:      

        ORMTimestamp   effective_date;        
        ORMTimestamp   last_updated;        
        ORMFloat       rate;        
        ORMString      rate_type;        
        ORMInt         tag_pricing;        
        TagPricing tag_pricing_object() const;        

        const ORMInt::value_type& id () const { return *id_;}        

        SiteRate (DB::IConn& connection);      
        SiteRate (const SiteRate& from);      
        SiteRate& operator=(const SiteRate& from);      

        SiteRate (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const SiteRate& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const SiteRate& val);
      
      class Sizetype:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[10];      

        ORMInt         size_type_id_;        

      public:      

        ORMFloat       flags;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      tag_templ_brpb_file;        
        ORMString      tag_templ_iest_file;        
        ORMString      tag_templ_iframe_file;        
        ORMString      tag_templ_preview_file;        
        ORMString      tag_template_file;        
        ORMTimestamp   version;        

        const ORMInt::value_type& size_type_id () const { return *size_type_id_;}        

        Sizetype (DB::IConn& connection);      
        Sizetype (const Sizetype& from);      
        Sizetype& operator=(const Sizetype& from);      

        Sizetype (DB::IConn& connection,        
          const ORMInt::value_type& size_type_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& size_type_id)        
        {        
          size_type_id_ = size_type_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& size_type_id, bool set_defaults = true)        
        {        
          size_type_id_ = size_type_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& size_type_id)        
        {        
          size_type_id_ = size_type_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& size_type_id)        
        {        
          size_type_id_ = size_type_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Sizetype& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Sizetype& val);
      
      class TagPricing:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[9];      

        ORMInt         id_;        

      public:      

        ORMString      ccg_rate_type;        
        ORMString      ccg_type;        
        ORMString      country_code;        
        Country country_code_object() const;        
        ORMTimestamp   last_updated;        
        ORMInt         site_rate;        
        SiteRate site_rate_object() const;        
        ORMString      status;        
        ORMInt         tag;        
        Tags tag_object() const;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        TagPricing (DB::IConn& connection);      
        TagPricing (const TagPricing& from);      
        TagPricing& operator=(const TagPricing& from);      

        TagPricing (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const TagPricing& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const TagPricing& val);
      
      class Tags:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[13];      

        ORMInt         id_;        

      public:      

        ORMBool        allow_expandable;        
        ORMInt         flags;        
        ORMTimestamp   last_updated;        
        ORMString      marketplace;        
        ORMString      name;        
        ORMString      passback;        
        ORMString      passback_code;        
        ORMString      passback_type;        
        ORMInt         site;        
        Site site_object() const;        
        ORMInt         size_type_id;        
        ORMString      status;        
        ORMTimestamp   version;        

        const ORMInt::value_type& id () const { return *id_;}        

        Tags (DB::IConn& connection);      
        Tags (const Tags& from);      
        Tags& operator=(const Tags& from);      

        Tags (DB::IConn& connection,        
          const ORMInt::value_type& id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& id, bool set_defaults = true)        
        {        
          id_ = id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& id)        
        {        
          id_ = id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Tags& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Tags& val);
      
      class Tagscreativecategoryexclusion:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[4];      

        ORMInt         creative_category_id_;        
        ORMInt         tag_id_;        

      public:      

        ORMString      approval;        
        ORMTimestamp   last_updated;        

        const ORMInt::value_type& creative_category_id () const { return *creative_category_id_;}        
        const ORMInt::value_type& tag_id () const { return *tag_id_;}        

        Tagscreativecategoryexclusion (DB::IConn& connection);      
        Tagscreativecategoryexclusion (const Tagscreativecategoryexclusion& from);      
        Tagscreativecategoryexclusion& operator=(const Tagscreativecategoryexclusion& from);      

        Tagscreativecategoryexclusion (DB::IConn& connection,        
          const ORMInt::value_type& creative_category_id,          
          const ORMInt::value_type& tag_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& tag_id)        
        {        
          creative_category_id_ = creative_category_id;          
          tag_id_ = tag_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& tag_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          tag_id_ = tag_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& tag_id, bool set_defaults = true)        
        {        
          creative_category_id_ = creative_category_id;          
          tag_id_ = tag_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& tag_id)        
        {        
          creative_category_id_ = creative_category_id;          
          tag_id_ = tag_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& creative_category_id,
          const ORMInt::value_type& tag_id)        
        {        
          creative_category_id_ = creative_category_id;          
          tag_id_ = tag_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Tagscreativecategoryexclusion& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Tagscreativecategoryexclusion& val);
      
      class Tag_tagsize:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         size_id_;        
        ORMInt         tag_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& size_id () const { return *size_id_;}        
        const ORMInt::value_type& tag_id () const { return *tag_id_;}        

        Tag_tagsize (DB::IConn& connection);      
        Tag_tagsize (const Tag_tagsize& from);      
        Tag_tagsize& operator=(const Tag_tagsize& from);      

        Tag_tagsize (DB::IConn& connection,        
          const ORMInt::value_type& size_id,          
          const ORMInt::value_type& tag_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& size_id,
          const ORMInt::value_type& tag_id)        
        {        
          size_id_ = size_id;          
          tag_id_ = tag_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& size_id,
          const ORMInt::value_type& tag_id, bool set_defaults = true)        
        {        
          size_id_ = size_id;          
          tag_id_ = tag_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& size_id,
          const ORMInt::value_type& tag_id, bool set_defaults = true)        
        {        
          size_id_ = size_id;          
          tag_id_ = tag_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& size_id,
          const ORMInt::value_type& tag_id)        
        {        
          size_id_ = size_id;          
          tag_id_ = tag_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& size_id,
          const ORMInt::value_type& tag_id)        
        {        
          size_id_ = size_id;          
          tag_id_ = tag_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Tag_tagsize& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Tag_tagsize& val);
      
      class Template:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[7];      

        ORMInt         template_id_;        

      public:      

        ORMBool        expandable;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      status;        
        ORMString      template_type;        
        ORMTimestamp   version;        

        const ORMInt::value_type& template_id () const { return *template_id_;}        

        Template (DB::IConn& connection);      
        Template (const Template& from);      
        Template& operator=(const Template& from);      

        Template (DB::IConn& connection,        
          const ORMInt::value_type& template_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& template_id)        
        {        
          template_id_ = template_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& template_id, bool set_defaults = true)        
        {        
          template_id_ = template_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& template_id)        
        {        
          template_id_ = template_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& template_id)        
        {        
          template_id_ = template_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Template& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Template& val);
      
      class Templatefile:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[9];      

        ORMInt         template_file_id_;        

      public:      

        ORMInt         app_format_id;        
        Appformat app_format_id_object() const;        
        ORMInt         flags;        
        ORMTimestamp   last_updated;        
        ORMInt         size_id;        
        Creativesize size_id_object() const;        
        ORMString      template_file;        
        ORMInt         template_id;        
        Template template_id_object() const;        
        ORMString      template_type;        
        ORMTimestamp   version;        

        const ORMInt::value_type& template_file_id () const { return *template_file_id_;}        

        Templatefile (DB::IConn& connection);      
        Templatefile (const Templatefile& from);      
        Templatefile& operator=(const Templatefile& from);      

        Templatefile (DB::IConn& connection,        
          const ORMInt::value_type& template_file_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& template_file_id)        
        {        
          template_file_id_ = template_file_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& template_file_id, bool set_defaults = true)        
        {        
          template_file_id_ = template_file_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& template_file_id)        
        {        
          template_file_id_ = template_file_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& template_file_id)        
        {        
          template_file_id_ = template_file_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Templatefile& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Templatefile& val);
      
      class Triggers:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[9];      

        ORMInt         trigger_id_;        

      public:      

        ORMString      channel_type;        
        ORMString      country_code;        
        ORMTimestamp   created;        
        ORMTimestamp   last_updated;        
        ORMString      normalized_trigger;        
        ORMString      qa_status;        
        ORMString      trigger_type;        
        ORMTimestamp   version;        

        const ORMInt::value_type& trigger_id () const { return *trigger_id_;}        

        Triggers (DB::IConn& connection);      
        Triggers (const Triggers& from);      
        Triggers& operator=(const Triggers& from);      

        Triggers (DB::IConn& connection,        
          const ORMInt::value_type& trigger_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& trigger_id)        
        {        
          trigger_id_ = trigger_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& trigger_id, bool set_defaults = true)        
        {        
          trigger_id_ = trigger_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& trigger_id)        
        {        
          trigger_id_ = trigger_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& trigger_id)        
        {        
          trigger_id_ = trigger_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Triggers& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Triggers& val);
      
      class Wdrequestmapping:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[6];      

        ORMInt         wd_req_mapping_id_;        

      public:      

        ORMString      description;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      request;        
        ORMTimestamp   version;        

        const ORMInt::value_type& wd_req_mapping_id () const { return *wd_req_mapping_id_;}        

        Wdrequestmapping (DB::IConn& connection);      
        Wdrequestmapping (const Wdrequestmapping& from);      
        Wdrequestmapping& operator=(const Wdrequestmapping& from);      

        Wdrequestmapping (DB::IConn& connection,        
          const ORMInt::value_type& wd_req_mapping_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& wd_req_mapping_id)        
        {        
          wd_req_mapping_id_ = wd_req_mapping_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& wd_req_mapping_id, bool set_defaults = true)        
        {        
          wd_req_mapping_id_ = wd_req_mapping_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& wd_req_mapping_id)        
        {        
          wd_req_mapping_id_ = wd_req_mapping_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& wd_req_mapping_id)        
        {        
          wd_req_mapping_id_ = wd_req_mapping_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const Wdrequestmapping& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const Wdrequestmapping& val);
      
      class WDTag:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[12];      

        ORMInt         wdtag_id_;        

      public:      

        ORMFloat       height;        
        ORMTimestamp   last_updated;        
        ORMString      name;        
        ORMString      opted_in_content;        
        ORMString      opted_out_content;        
        ORMString      passback;        
        ORMInt         site_id;        
        Site site_id_object() const;        
        ORMString      status;        
        ORMInt         template_id;        
        Template template_id_object() const;        
        ORMTimestamp   version;        
        ORMFloat       width;        

        const ORMInt::value_type& wdtag_id () const { return *wdtag_id_;}        

        WDTag (DB::IConn& connection);      
        WDTag (const WDTag& from);      
        WDTag& operator=(const WDTag& from);      

        WDTag (DB::IConn& connection,        
          const ORMInt::value_type& wdtag_id          
        );        

        virtual bool touch (); //!< touch version      

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& wdtag_id)        
        {        
          wdtag_id_ = wdtag_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& wdtag_id, bool set_defaults = true)        
        {        
          wdtag_id_ = wdtag_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& wdtag_id)        
        {        
          wdtag_id_ = wdtag_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& wdtag_id)        
        {        
          wdtag_id_ = wdtag_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const WDTag& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const WDTag& val);
      
      class WDTagfeed_optedin:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         wdtag_id_;        
        ORMInt         feed_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& wdtag_id () const { return *wdtag_id_;}        
        const ORMInt::value_type& feed_id () const { return *feed_id_;}        

        WDTagfeed_optedin (DB::IConn& connection);      
        WDTagfeed_optedin (const WDTagfeed_optedin& from);      
        WDTagfeed_optedin& operator=(const WDTagfeed_optedin& from);      

        WDTagfeed_optedin (DB::IConn& connection,        
          const ORMInt::value_type& wdtag_id,          
          const ORMInt::value_type& feed_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedin& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedin& val);
      
      class WDTagfeed_optedout:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[3];      

        ORMInt         wdtag_id_;        
        ORMInt         feed_id_;        

      public:      

        ORMTimestamp   last_updated;        

        const ORMInt::value_type& wdtag_id () const { return *wdtag_id_;}        
        const ORMInt::value_type& feed_id () const { return *feed_id_;}        

        WDTagfeed_optedout (DB::IConn& connection);      
        WDTagfeed_optedout (const WDTagfeed_optedout& from);      
        WDTagfeed_optedout& operator=(const WDTagfeed_optedout& from);      

        WDTagfeed_optedout (DB::IConn& connection,        
          const ORMInt::value_type& wdtag_id,          
          const ORMInt::value_type& feed_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id, bool set_defaults = true)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return delet ();        
        }        
        bool del  (const ORMInt::value_type& wdtag_id,
          const ORMInt::value_type& feed_id)        
        {        
          wdtag_id_ = wdtag_id;          
          feed_id_ = feed_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedout& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const WDTagfeed_optedout& val);
      
      class WebwiseDiscoverItem:      
        public ORMObject<postgres_connection>      
      {      
      protected:      
        static ORMObjectMember members_[5];      

        ORMString      item_id_;        

      public:      

        ORMString      language;        
        ORMString      link;        
        ORMTimestamp   pub_date;        
        ORMString      title;        

        const ORMString::value_type& item_id () const { return *item_id_;}        

        WebwiseDiscoverItem (DB::IConn& connection);      
        WebwiseDiscoverItem (const WebwiseDiscoverItem& from);      
        WebwiseDiscoverItem& operator=(const WebwiseDiscoverItem& from);      

        WebwiseDiscoverItem (DB::IConn& connection,        
          const ORMString::value_type& item_id          
        );        

        virtual bool select (); //!< get exists      
        bool select (const ORMString::value_type& item_id)        
        {        
          item_id_ = item_id;          
          return select();        
        }
        
        virtual bool update (bool set_defaults = true); //!< update exists      
        bool update (const ORMString::value_type& item_id, bool set_defaults = true)        
        {        
          item_id_ = item_id;          
          return update(set_defaults);        
        }
        
        virtual bool insert (bool set_defaults = true); //!< create new      
        bool insert (const ORMString::value_type& item_id, bool set_defaults = true)        
        {        
          item_id_ = item_id;          
          return insert(set_defaults);        
        }
        
        virtual bool delet  (); //!< delete exists      
        virtual bool del    (); //!< set status D and name->name+timestamp      
        virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record      
        bool delet  (const ORMString::value_type& item_id)        
        {        
          item_id_ = item_id;          
          return delet ();        
        }        
        bool del  (const ORMString::value_type& item_id)        
        {        
          item_id_ = item_id;          
          return del ();        
        }        
      friend std::ostream& operator<< (std::ostream& out, const WebwiseDiscoverItem& val);      
      };      
      std::ostream& operator<< (std::ostream& out, const WebwiseDiscoverItem& val);
      
      inline Accounttype Account::account_type_id_object() const      
      {      
        return Accounttype(conn, account_type_id.value());      
      }      
      inline Country Account::country_code_object() const      
      {      
        return Country(conn, country_code.value());      
      }      
      inline Currency Account::currency_object() const      
      {      
        return Currency(conn, currency.value());      
      }      
      inline Accountrole Accounttype::account_role_id_object() const      
      {      
        return Accountrole(conn, account_role_id.value());      
      }      
      inline Account Action::account_id_object() const      
      {      
        return Account(conn, account_id.value());      
      }      
      inline Behavioralparameterslist BehavioralParameters::behav_params_list_id_object() const      
      {      
        return Behavioralparameterslist(conn, behav_params_list_id.value());      
      }      
      inline Channel BehavioralParameters::channel_object() const      
      {      
        return Channel(conn, channel.value());      
      }      
      inline Account Campaign::account_object() const      
      {      
        return Account(conn, account.value());      
      }      
      inline FreqCap Campaign::freq_cap_object() const      
      {      
        return FreqCap(conn, freq_cap.value());      
      }      
      inline CampaignCreativeGroup CampaignCreative::ccg_object() const      
      {      
        return CampaignCreativeGroup(conn, ccg.value());      
      }      
      inline Creative CampaignCreative::creative_object() const      
      {      
        return Creative(conn, creative.value());      
      }      
      inline FreqCap CampaignCreative::freq_cap_object() const      
      {      
        return FreqCap(conn, freq_cap.value());      
      }      
      inline Campaign CampaignCreativeGroup::campaign_object() const      
      {      
        return Campaign(conn, campaign.value());      
      }      
      inline CCGRate CampaignCreativeGroup::ccg_rate_object() const      
      {      
        return CCGRate(conn, ccg_rate.value());      
      }      
      inline Channel CampaignCreativeGroup::channel_object() const      
      {      
        return Channel(conn, channel.value());      
      }      
      inline Country CampaignCreativeGroup::country_code_object() const      
      {      
        return Country(conn, country_code.value());      
      }      
      inline FreqCap CampaignCreativeGroup::freq_cap_object() const      
      {      
        return FreqCap(conn, freq_cap.value());      
      }      
      inline CampaignCreativeGroup CCGKeyword::ccg_object() const      
      {      
        return CampaignCreativeGroup(conn, ccg.value());      
      }      
      inline Channel CCGKeyword::channel_id_object() const      
      {      
        return Channel(conn, channel_id.value());      
      }      
      inline CampaignCreativeGroup CCGRate::ccg_object() const      
      {      
        return CampaignCreativeGroup(conn, ccg.value());      
      }      
      inline Account Channel::account_object() const      
      {      
        return Account(conn, account.value());      
      }      
      inline Behavioralparameterslist Channel::behav_params_list_id_object() const      
      {      
        return Behavioralparameterslist(conn, behav_params_list_id.value());      
      }      
      inline Channelrate Channel::channel_rate_id_object() const      
      {      
        return Channelrate(conn, channel_rate_id.value());      
      }      
      inline Country Channel::country_code_object() const      
      {      
        return Country(conn, country_code.value());      
      }      
      inline FreqCap Channel::freq_cap_id_object() const      
      {      
        return FreqCap(conn, freq_cap_id.value());      
      }      
      inline Channel Channelrate::channel_id_object() const      
      {      
        return Channel(conn, channel_id.value());      
      }      
      inline Currency Channelrate::currency_id_object() const      
      {      
        return Currency(conn, currency_id.value());      
      }      
      inline Channel Channeltrigger::channel_id_object() const      
      {      
        return Channel(conn, channel_id.value());      
      }      
      inline Triggers Channeltrigger::trigger_id_object() const      
      {      
        return Triggers(conn, trigger_id.value());      
      }      
      inline Account Colocation::account_object() const      
      {      
        return Account(conn, account.value());      
      }      
      inline ColocationRate Colocation::colo_rate_object() const      
      {      
        return ColocationRate(conn, colo_rate.value());      
      }      
      inline Colocation ColocationRate::colo_object() const      
      {      
        return Colocation(conn, colo.value());      
      }      
      inline Currency Country::currency_id_object() const      
      {      
        return Currency(conn, currency_id.value());      
      }      
      inline Account Creative::account_object() const      
      {      
        return Account(conn, account.value());      
      }      
      inline Creativesize Creative::size_object() const      
      {      
        return Creativesize(conn, size.value());      
      }      
      inline Template Creative::template_id_object() const      
      {      
        return Template(conn, template_id.value());      
      }      
      inline Creativecategorytype CreativeCategory::type_object() const      
      {      
        return Creativecategorytype(conn, type.value());      
      }      
      inline Account Site::account_object() const      
      {      
        return Account(conn, account.value());      
      }      
      inline FreqCap Site::freq_cap_object() const      
      {      
        return FreqCap(conn, freq_cap.value());      
      }      
      inline Sitecategory Site::site_category_id_object() const      
      {      
        return Sitecategory(conn, site_category_id.value());      
      }      
      inline Country Sitecategory::country_code_object() const      
      {      
        return Country(conn, country_code.value());      
      }      
      inline TagPricing SiteRate::tag_pricing_object() const      
      {      
        return TagPricing(conn, tag_pricing.value());      
      }      
      inline Country TagPricing::country_code_object() const      
      {      
        return Country(conn, country_code.value());      
      }      
      inline SiteRate TagPricing::site_rate_object() const      
      {      
        return SiteRate(conn, site_rate.value());      
      }      
      inline Tags TagPricing::tag_object() const      
      {      
        return Tags(conn, tag.value());      
      }      
      inline Site Tags::site_object() const      
      {      
        return Site(conn, site.value());      
      }      
      inline Appformat Templatefile::app_format_id_object() const      
      {      
        return Appformat(conn, app_format_id.value());      
      }      
      inline Creativesize Templatefile::size_id_object() const      
      {      
        return Creativesize(conn, size_id.value());      
      }      
      inline Template Templatefile::template_id_object() const      
      {      
        return Template(conn, template_id.value());      
      }      
      inline Site WDTag::site_id_object() const      
      {      
        return Site(conn, site_id.value());      
      }      
      inline Template WDTag::template_id_object() const      
      {      
        return Template(conn, template_id.value());      
      }      
    }
  }
}

#endif // _AUTOTEST_COMMONS_ORM_PQORMOBJECTS_HPP
