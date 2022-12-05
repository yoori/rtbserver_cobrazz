
#ifndef __AUTOTEST_COMMONS_ADMINS_HPP
#define __AUTOTEST_COMMONS_ADMINS_HPP

#include <iostream>

#include <String/StringManip.hpp>

#include <tests/AutoTests/Commons/Utils.hpp>
#include <tests/AutoTests/Commons/Logger.hpp>
#include "Command.hpp"

namespace AutoTest
{
  static const char ANY[] = ".*";

  /**
   * @class ShellCmd
   * @brief Basic class for executing shell commands
   * with interface to examine result.
   *
   * This class is constructed to execute shell commands,
   * fetch results of this commands (standard output) and analyse it.
   * Analysis of result perform special external component - examiner,
   * which take istream as argument and
   * transform it into internal representation.
   * Interface for this component you can
   * find in ShellCmd class body (class Examiner).
   * You can make your own examiner for any shell command output.
   */
  class ShellCmd
  {
  public:
    /**
     * @brief Declare CmdStatusFailed exception.
     *
     * Exception raised when executing shell command failed
     * and command returns non zero status code.
     */
    DECLARE_EXCEPTION(CmdStatusFailed, eh::DescriptiveException);
    //DECLARE_EXCEPTION(BadCommand, eh::DescriptiveException);   -   unused

  protected:
    typedef std::list<std::string> cmdlist;  //!< Type of command.
    typedef cmdlist::const_iterator cmdliter;//!< Type of command iterator.

    /**
     * @brief Shell command string.
     *
     * Command for executing in shell. Represents list of strings.
     * First element of list is command name,
     * others are keys and parameters of this command.
     */
    cmdlist  cmd_;

    /**
     * @brief Interface for examiner.
     *
     * All examiners must have that interface for using with ShellCmd class.
     */
    class Examiner
    {
    public:

      /**
       * @brief Destructor.
       */
      virtual ~Examiner () {}

      /**
       * @brief Examine input stream.
       */
      virtual bool examine (std::istream& in) = 0;
    };

    class DumpExaminer: public Examiner
    {
      Logger& logger;
      unsigned long log_level;
    public:
      /**
       * @brief Cpnstructor
       * @param logger
       * @param log_level
       */
      DumpExaminer(Logger& logger_,
                   unsigned long log_level_);

      /**
       * @brief Examine input stream.
       */
      bool examine (std::istream& in);
    };

    /**
     * @brief Interface for dump examiner.
     *
     * Help to dump admin's output to log.
     */
    /**
     * @brief Execute shell command and examine result.
     *
     * Create child process which execute shell command,
     * then fetch result using pipe (output of shell command) and examine it.
     * @param examiner examiner to analyse result.
     * @return true if examination is successful and there is no errors
     * of executing shell command and fetching result.
     * @note This function uses system calls and if any of this calls failed
     * function will return false.
     */
    bool fetch(Examiner& examiner) const;

  public:

    /**
     * @brief Clear command string.
     *
     * Makes command line empty and you can add new command to execute.
     */
    void clear ()
    {
      cmd_.clear();
    }

    /**
     * @brief Adds command name or others command attributes (keys or arguments).
     *
     * Use this function to build command line. 
     * @param cmd pointer to null-terminated string that represents
     * command name or command argument.
     * @return true if adding command is successful, false otherwise.
     */
    bool add_cmd_i (const char* cmd)
    {
      if(strlen(cmd) == 0)
      {
        return false;
      }
      cmd_.push_back(cmd);
      return true;
    }

    /**
     * @brief Adds command name or others command attributes (keys or arguments).
     *
     * Use this function to build command line.
     * @param cmd STL string that represents
     * command name or command argument.
     * @return true if adding command is successful, false otherwise.
     */
    bool add_cmd_i (const std::string& cmd)
    {
      if(cmd.empty() || !add_cmd_i(cmd.c_str()))
      {
        return false;
      }
      return true;
    }

    /**
     * @brief Default constructor.
     */
    ShellCmd ()
    {
    }

    /**
     * @brief Default destructor.
     */
    virtual ~ShellCmd ()
    {
    }

    /**
     * @brief Executes command without examination.
     */
    void exec();

    /**
     * @brief Prints shell command.
     *
     * Outputs command line into the ostream.
     * @param out stream to out command line.
     * @return reference to out stream.
     */
    std::ostream& print_idname (std::ostream& out) const;

    /**
     * @brief Prints result of executing shell command (command output).
     *
     * Executes shell command and dump result into ostream without examine it.
     * @param out stream to out result of executing shell command.
     * @return reference to out stream.
     */
    std::ostream&  dump_native_out (std::ostream& out) const;

    /**
     * @brief Logs admin output.
     *
     * Executes admin command and logs it's output.
     * Throw exception in case of command returns non zero status code.
     * @param logger logger to log admin output.
     * @param log_level logging level.
     */
    void log(Logger& logger,
      unsigned long log_level = Logging::Logger::DEBUG,
      bool rethrow = false);
  };

  typedef std::map<unsigned long, ComparableVar> FieldIndexMap;
  //typedef std::map<unsigned long, std::string> FieldIndexMap;

  /**
   * @class AdminCmd
   * @brief ShellCmd with examiner for analyse output of Admin command.
   *
   * This class is for execurte Admin shell command,
   * fetch results and examine it.
   * Admin shell command returns information
   * about objects loaded to AdServer memory.
   * To choose type of object call Admin command with corresponding aspect.
   * Also you can indicate objects ids through a comma (aspect value) to
   * output information about only several necessary objects.
   * This objects consist of fields that have name and value
   * and Admin output this fields in format: 'filed name: value'.
   * Class have its own examiner for examine results.
   * Base on ShellCmd class.
   */
  class AdminCmd:
    public ShellCmd
  {
  protected:

    /**
     * @brief CORBA address for Admin.
     *
     * Defines location of the AdServer service (CampaignManager
     * or ChannelManagerController for example) that is responsible
     * for objects of some kind of type.
     * Represents address string with the following format:
     * "host_address:port_number".
     */
    std::string address_;

  public:
    typedef std::string              value_type;
    typedef std::vector<std::string> values_type;
    typedef std::vector<values_type> values_list_type;
    typedef ComparableList expects_type;
    typedef std::vector<expects_type> expects_list_type;
    typedef std::vector<bool>        mask_type;
  protected:
    /**
     * @brief List of actual fileds names of some object.
     *
     * Only fields with this names will be fetched from command output and
     * compared with expected values.
     */
    values_type names_;

    /**
     * @brief List of objects shown by Admin (list of result values).
     *
     * Fetched from Admin command output.
     */
    values_list_type values_;

    /**
     * @brief List of expected values.
     */
    expects_list_type expects_;

    /**
     * @brief Mask of found expected values.
     *
     * This mask shows whether some expected value was found in Admin output.
     * Mask field with some index contains true
     * if corresponding expected value with the same index
     * appear in Admin output.
     */
    mask_type finds_;

    /**
     * @brief Prints list of values.
     *
     * @param out stream to out values.
     * @param values list of values.
     * @return reference to out stream.
     */
    std::ostream& dumpout (
      std::ostream& out,
      const values_list_type& values) const;
    std::ostream& dumpout (
      std::ostream& out,
      const expects_list_type& values) const;

    /**
     * @brief Check some result value.
     *
     * Checks whether result value of some object's field equal to
     * corresponding expected value.
     * @param expects expected value.
     * @param i index of result value that is compared with expected one.
     * @return true if result and expected values are equal, false otherwise.
     * @note Function checks only the last object in the list of result values.
     */
    bool check (const Comparable* expects, size_t i);
    /**
     * @brief Check appearance of expected values in Admin command output.
     *
     * Compare expected values with the last fetched from Admin output value.
     * Result of comparison writes to the finds_ mask.
     * @return true if all expected values appear in the Admin output
     * i.e. all fields of mask finds_ contain true.
     */
    bool check ();

    /**
     * @brief Add new object into the list of result values.
     *
     * This fuction is used to fill list of result values
     * on basis of command output.
     * @param values added values
     */
    void add (const values_type& values);

    /**
     * @brief Sets actual fileds names.
     *
     * @param count amount of new names.
     * @param names new names.
     */
    void setup (size_t count, const char* const* names);
    
  public:

    /**
     * @brief Number of actual object's fileds.
     * @return size of list of fields names
     */
    size_t slice_size () const { return names_.size();}

    /**
     * @brief Shows number of returned by Admin objects.
     *
     * @return list of result values size.
     */
    size_t size () const { return values_.size();}

    /**
     * @brief Check whether the list of result values is empty.
     *
     * @return true if list of result values is emplty, false otherwise.
     */
    bool empty () const { return values_.empty();}

    /**
     * @brief Clear values.
     *
     * Makes values list empty.
     */
    void clear () { values_.clear();}

    /**
     * @brief Get last element of list
     * of result values (for read/write purposes).
     *
     * @return nonconst reference to the last element of list of result values,
     * so you can modify this element using returned value.
     */
    values_type& last () { return values_.back();}

    /**
     * @brief Get last elemet of list
     * of result values (for read only purposes).
     *
     * @return const reference to the last element of list of result values,
     * so you can't modify this element using returned value.
     */
    const values_type& last () const { return values_.back();}

    /**
     * @brief Access to the element of result values list.
     *
     * Provides read/write access to the one of elements
     * of result values list by index.
     * Operator [] does not check whether the index
     * passed as an argument is valid
     * @param i - element index.
     * @return reference to the object (element of result values list)
     * at the position of the passed index.
     */
    values_type& operator[] (size_t i)
    {
      return values_[i];
    }

    /**
     * @brief Access to the element of result values list. Constant version.
     *
     * Provides read-only access to the one of elements
     * of result values list by index.
     * Operator [] does not check whether the index
     * passed as an argument is valid
     * @param i - element index.
     * @return const reference to the object at the position of the passed index.
     */
    const values_type& operator[] (size_t i) const
    {
      return values_[i];
    }

    /**
     * @brief Access to the admin field (column) names.
     *
     * Provides read-only access to admin field name
     * @param i - element index.
     * @return const reference to the field name string
     */
    const std::string& field_name (size_t i) const
    {
      return names_[i];
    }

    /**
     * @brief Access to the service address.
     *
     * Provides read-only access to the service adress
     * @return const reference service address string
     */
    const std::string& address() const
    {
      return address_;
    }

    /**
     * @brief Define interface for examiner.
     */
    typedef ShellCmd::Examiner BasicExaminer;

  protected:
    /**
     * @class Examiner
     * @brief Base examiner for AdminCmd classes.
     *
     * Examiner analysys results of executing Admin commands.
     * The aim is to compare actual values fetched from Admin command output
     * with expected values using some rules of comparison.
     * @note All Admins have the similar output format.
     */
    class Examiner: public BasicExaminer
    {
    protected:
      
      /**
       * @brief Admin command to examine.
       */
      AdminCmd& admin_;

      /**
       * @brief Whether to fetch all values to internal structure
       *
       * If false - stop fetching values from admin output
       * to internal structure when all expects founded.
       */
      bool fetch_all_;

      typedef std::vector<std::string> values_type;

      /**
       * @brief List of values fetched from Admin output.
       *
       * This values represent one object.
       */
      values_type values_;

      /**
       * @brief Check whether expected values
       * appear in the list of result values.
       *
       * This function checks whether fields values
       * of some object are not empty, add it to the admin_'s list
       * of result values and call AdminCmd::check().
       * @return true if all expected values appear in the Admin output
       * (list of result values).
       */
      bool check ();

      /**
       * @brief Set values_.
       *
       * Set new value for the field with indicated name.
       * Used to save only actual fetched values in internal structures.
       * @param name field name.
       * @param value field value.
       */
      void set (const char* name, const char* value);
      
    public:
      
      /**
       * @brief Constructor.
       *
       * Create examiner for admin.
       * @param admin Admin command to examine.
       */
      Examiner(AdminCmd& admin, bool fetch_all = false);

      /**
       * @brief Examine Admin output.
       *
       * This function analysys Admin output.
       * It fetchs data from input stream and save it in
       * internal structures. Then check whether all expected values
       * appear in the Admin output.
       * @param in analyzed input stream.
       * @return true if examiner gets all expected values.
       */
      virtual bool examine (std::istream& in);
      
    };

    class SingleValueExaminer:
      public BasicExaminer
    {
    protected:

      AdminCmd& admin_;

    public:
      SingleValueExaminer (AdminCmd& admin): admin_(admin)
      {};

      virtual bool examine (std::istream& in);
    };

    /**
     * @brief Setup expected values.
     *
     * Setup expected fileds names and values.
     * Examiner will search this values in the Admin otput.
     * @param examiner examiner for Admin.
     * @param size amount of name-value pairs to setup.
     * @param nvpair list of name-value pairs to setup
     */
    bool setup_examine(BasicExaminer& examiner,
      size_t size,
      const std::string* nvpair);

  public:

    /**
     * @brief Virtual function to make call string for admin.
     */
    virtual void make_cmd (const char* /*aspect_value*/) {};

    /**
     * @brief Make call string for admin using aspect value.
     *
     * @param aspect_value aspect value 
     */
    void cmd (const std::string& aspect_value)
    {
      make_cmd(aspect_value.c_str());
    }

    /**
     * @brief Make call string for admin using aspect value.
     *
     * @param aspect_value aspect value
     * @param address CORBA address of AdServer service.
     */
    void cmd (const char* aspect_value, const char* address)
    {
      address_ = address;
      make_cmd(aspect_value);
    }

    /**
     * @brief Make call string for admin using aspect value.
     *
     * @param aspect_value aspect value
     * @param address CORBA address of AdServer service.
     */
    void cmd (const std::string& aspect_value, const char* address)
    {
      address_ = address;
      make_cmd(aspect_value.c_str());
    }

    /**
     * @brief Default constructor.
     */
    AdminCmd (unsigned long skip_lines_ = 0)
      : skip_lines(skip_lines_)
    {}

    /**
     * @brief Examine Admin output.
     *
     * Executes Admin command and analyses output of this command
     * using examiner. Expected values must be set for this Admin.
     * @param examiner examiner to analyse result.
     * @return true if command executed with success and all
     * expected values present in output.
     */
    bool examine(BasicExaminer& examiner);

    /**
     * @brief Setup expected values and examine Admin output.
     *
     * Execute admin command and examine results.
     * List of expected values may not be set,
     * it's received as a function parameter.
     * @note list of expected values represents one expected object.
     * @param examiner examiner to examine Admin output.
     * @param size size of list of expected values.
     * @param expects list of expected values.
     * @return true if command executing is successful
     * and all expected values received.
     */
    bool examine(
      BasicExaminer& examiner, 
      size_t size, 
      const char* const* expects);

    bool examine(const char* expects);

    /**
     * @brief Setup expected values and examine Admin output.
     *
     * This function is like previous one, but it uses list of STL strings
     * (instead of list of C-strings) as a list of expected values.
     */
    bool examine(
      BasicExaminer& examiner,
      size_t size,
      const std::string expects[]);

    /**
     * @brief Setup expected values and examine Admin output.
     *
     * Execute admin command and examine results.
     * Receive list of expected values as a function parameter.
     * @note list of expected values
     * can represent more than one expected object.
     * @param examiner examiner to examine Admin output.
     * @param sizex number of expected objects.
     * @param size number of fileds for the obeject.
     * @param expects list of expected values.
     * @return true if command execution and examination are successful.
     */
    bool examine(
      BasicExaminer& examiner,
      size_t sizex,
      size_t size,
      const std::string* expects);

    /**
     * @brief Setup expected values and examine Admin output.
     *
     * This function is like previous one, but it uses list of STL strings
     * (instead of list of C-strings) as a list of expected values.
     */
    bool examine(
      BasicExaminer& examiner,
      size_t sizex,
      size_t size,
      const char* const* expects);

    /**
     * @brief Prints Admin command call string.
     *
     * Output Admin call string into out stream.
     * @param out stream to out command call string.
     * @return reference to outstream.
     */
    std::ostream& print_idname (std::ostream& out) const
    {
      return ShellCmd::print_idname(out);
    }

    /**
     * @brief Prints result of executing Admin command.
     *
     * Dumps result values into ostream.
     * @param out stream to out result values.
     * @return reference to out stream.
     */
    std::ostream& dumpout (std::ostream& out) const;

    /**
     * @brief Prints expected values.
     *
     * Dumps list of expected values into ostream.
     * @param out stream to out expected values.
     * @return reference to out stream.
     */
    std::ostream& dump_expects (std::ostream& out) const;

    typedef std::string name_value_type1[2];  //!< Name-value pair.

    /**
     * @brief Setup expected values and examine Admin output.
     *
     * Setup fields names and fields values of expected object,
     * execute Admin command and examine results.
     * @param nvpairs expected name-value pairs.
     * @return true if examination is successful and all
     * expected values appear in Admin output.
     */
    template<size_t Count>
    bool examine(const name_value_type1 (&nvpairs)[Count])
    {
      Examiner examiner(*this);
      return setup_examine(examiner, Count*2, static_cast<std::string*>(nvpairs));
    }

    typedef const char* name_value_type2[2];  //!< Name-value pair.

  protected:
    bool check_(size_t size, const char* const* names, const FieldIndexMap& expected) /*const*/
      /*throw(eh::Exception)*/;
  public:
    unsigned long skip_lines;
  };

  /**
   * @class BaseAdminCmd
   * @brief Basic class to implement shell result objects
   * provide infrastructure for expectation.
   */
  template<class Tag, size_t Count, class BaseCmd = AdminCmd>
  class BaseAdminCmd:
    public BaseCmd
  {
  public:
    typedef const char* names_type[Count];
    typedef const std::string expects_type1[Count];
    typedef const char* expects_type2[Count];

    static const unsigned long FIELDS_COUNT = Count;

  protected:
    static const names_type field_names;          //!< Object fields names
    typedef typename BaseCmd::Examiner Examiner;

  public:
    /**
     * @brief Default constructor.
     *
     * Creates BaseAdminCmd object and setup fields names for it.
     */
    BaseAdminCmd (unsigned long skip_lines = 0)
      : BaseCmd(skip_lines)
    {
      BaseCmd::setup(Count, field_names);
    }

    /**
     * @brief Setup expected values and examine Admin output.
     * @param expects list of expected values.
     * @return whether all expected values appear in the Admin output.
     * @note list of expected values represents one object.
     */
    bool examine(const expects_type1& expects)
    {
      Examiner examiner(*this);
      return BaseCmd::examine(examiner, Count, expects);
    }

    /**
     * This function is like previos one,
     * but uses list of C-strings (instead of STL-strings) for the argument.
     */
    bool examine(const expects_type2& expects)
    {
      Examiner examiner(*this);
      return BaseCmd::examine(examiner, Count, expects);
    }

    bool examine(const char* expects)
    {
      return BaseCmd::examine(expects);
    }

    /**
     * @brief Setup expected values and examine Admin output.
     * @param expects list of expected values.
     * @return whether all expected values appear in the Admin output.
     * @note list of expected values can represent more than one object.
     */
    template<size_t Count2>
    bool examine(const expects_type1 (&expects)[Count2])
    {
      Examiner examiner(*this);
      const expects_type1* array1 = 
        static_cast<const expects_type1*>(expects);
      const std::string* array2 = 
        static_cast<const std::string*>(*array1);
      return BaseCmd::examine(examiner,
        Count2,
        Count,
        array2); // REVIEW !!!
    }

    /**
     * This function is like previos one,
     * but uses list of C-strings (instead of STL-strings) for the argument.
     */
    template<size_t Count2>
    bool examine(const expects_type2 (&expects)[Count2])
    {
      Examiner examiner(*this);
      return BaseCmd::examine(examiner,
        Count2,
        Count,
        static_cast<const char* const*>(expects));
    }

    /**
     * @brief Executes Admin command and examines results.
     * @sa ShellCmd::fetch().
     */
    bool fetch(bool fetch_all = true)
    {
      BaseCmd::clear();
      Examiner examiner(*this, fetch_all);
      return BaseCmd::fetch(examiner);
    }

    /**
     * @brief Prints Admin command call string.
     *
     * @sa AdminCmd::print_idname().
     */
    std::ostream& print_idname(std::ostream& out)
    {
      return BaseCmd::print_idname(out);
    }

    /**
     * @brief Dumps result values list into ostream.
     *
     * @sa AdminCmd::dumpout().
     */
    std::ostream& dumpout(std::ostream& out)
    {
      return BaseCmd::dumpout(out);
    }

    /**
     * @brief Dumps expected values list into ostream
     *
     * @sa AdminCmd::dump_expects().
     */
    std::ostream& dump_expects(std::ostream& out)
    {
      return BaseCmd::dump_expects(out);
    }
  protected:
    bool check_(const FieldIndexMap& expected_values)
      /*throw(eh::Exception)*/
    {
      return AdminCmd::check_(FIELDS_COUNT, field_names, expected_values);
    }


    bool check_(const std::string& expected) /*const*/
      /*throw(eh::Exception)*/
   { 
      return examine(expected.c_str());
    }

  };

  std::string
  get_field_list(const AdminCmd& adm, unsigned int field);

}//namespace AutoTest

#endif //__AUTOTEST_COMMONS_ADMINS_HPP
