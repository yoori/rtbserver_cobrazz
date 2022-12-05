#include <stdio.h>
#include <ext/stdio_filebuf.h>

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  bool ShellCmd::fetch(Examiner& examiner) const
  {
    FILE* tmp_file = tmpfile();
    if (tmp_file == NULL)
    {
      int error = errno;
      Logger::thlog().error("tmpfile(): failed to create tmp file: " + strof(error));
      return false;
    }
    int efile = fileno(tmp_file);
    if(efile < 0)
    {
      int error = errno;
      Logger::thlog().error("fileno(): failed to get fileno of tmp file: " + strof(error));
      return false;
    }
    int output_[2];
    if (pipe(output_) < 0)
    {
      int error = errno;
      Logger::thlog().error("pipe(): failed to create pipe: " + strof(error));
      return false;//throw
    }
    if(!cmd_.empty())
    {
      std::string cmd_string("ShellCmd: going to exec:");
      for(cmdliter i = cmd_.begin(); i != cmd_.end(); ++i)
      {
        cmd_string += " " + *i;
      }

      Logger::thlog().debug_trace(cmd_string);
    }
    pid_t pid = fork();
    if (pid < 0)
    {
      int error = errno;
      Logger::thlog().error("fork(): failed to create process: " + strof(error));
      close(output_[0]);
      close(output_[1]);
      return false;//throw
    }
    else if (pid == 0) // child 
    {
      dup2(output_[1], STDOUT_FILENO);
      dup2(efile, STDERR_FILENO);
      fclose(tmp_file);
      char* argv[100];
      int i = 0;
      for(cmdliter j = cmd_.begin(); j != cmd_.end(); ++i, ++j)
      {
        argv[i] = const_cast<char*>(j->c_str()); //allow for execvp
      }
      argv[i] = 0;
      execvp(argv[0], argv);
      int error = errno;
      _exit(error);
    }
    else // parent
    {
      bool result;
      close(output_[1]);
      {
        __gnu_cxx::stdio_filebuf<char> buffer(output_[0], std::ios_base::in, 4096);
        std::istream stream(&buffer);
        result = examiner.examine(stream);
        while(!stream.eof()) stream.get();
        buffer.close();
      }
      int status;
      waitpid(pid, &status, 0);
      {
        lseek(efile, 0, SEEK_SET);
        __gnu_cxx::stdio_filebuf<char> ebuffer(efile,  std::ios_base::in, 4096);
        std::istream estream(&ebuffer);
        if(status)
        {
          Stream::Error ostr;    
          std::string line;
          ostr << "'";
          print_idname(ostr);
          ostr << "' status: "
               << WEXITSTATUS(status) << std::endl
               << " stderr: "
               << std::endl;
          while(!estream.eof())
          {
            getline(estream, line); 
            ostr << "  " << line << std::endl;
          }
          throw CmdStatusFailed(ostr);
        }
        while(!estream.eof()) estream.get();
        ebuffer.close();
      }
      return result;
    } // parent
  }

  void ShellCmd::exec()
  {
    class NullExaminer: public Examiner {
    public:
      bool examine (std::istream&) { return true;}
    } examiner;
    
    fetch(examiner);
  }

  std::ostream& ShellCmd::dump_native_out (std::ostream& out) const
  {
    class DumpExaminer: public Examiner
    {
      std::ostream& out;
    public:
      DumpExaminer(std::ostream& o):out(o) {}
      bool examine (std::istream& in) {
        char c;
        while (in.get(c))
        {
          out << c;
        }
        return true;
      }
    } examiner (out);

    fetch(examiner);

    return out;
  }

  std::ostream& ShellCmd::print_idname (std::ostream& out) const
  {
    for(cmdliter j = cmd_.begin(); j != cmd_.end(); ++j)
    {
      if (j != cmd_.begin()) out << ' ';
      out << *j;
    }
    return out;
  }

  AdminCmd::Examiner::Examiner(AdminCmd& admin, bool fetch_all)
    : admin_(admin), fetch_all_(fetch_all)
  {
    values_.resize(admin_.slice_size());
  }

  bool AdminCmd::Examiner::examine (std::istream& in)
  {
    values_.resize(admin_.slice_size());
    bool result = false;
    char name [256] = {0};
    char value[4096*16] = {0};

    // Skip garbage output
    for (unsigned int i=0; i < admin_.skip_lines; ++i)
    {
      while (!in.eof() && in.get() != '\n');
    }

  begin:
    while(!in.eof())
    {
      while(isspace(name[0] = in.get()))
      {
        if(name[0] == '\n')
        {
          if(check() && !result)
          {
            if (fetch_all_)
            {
              result = true;
            }
            else
            {
              return true;
            }
          }
          goto begin; // WHAT !!!???
        }
      }

      if(in.eof())
      {
        break;
      }

      in.getline(name+1, sizeof(name)-1, ' ');
      while((isspace(value[0] = in.get()) || value[0] == ':') && value[0] != '\n')
      {};

      if(value[0] != '\n')
      {
        if(isspace(value[0]))
        {
          while(isspace(value[0] = in.get()))
          {};
        }
        unsigned int i = 1;
        while(i < countof(value) && !in.eof() && ((value[i] = in.get()) != '\n'))
        {
          ++i;
        }

        if(i == countof(value))
        {
          Logger::thlog().error(std::string("AdminCmd: name='") +
            name + "' value is too long;");
          i = i - 1;
          if(value[i] != '\n')
          {
            while(!in.eof() && in.get() != '\n');
          }
        }

        value[i] = 0;
        while(i != 0 && ((value[i] == 0) || isspace(value[i]) ||
          (value[i] == '\n')))
        {
          value[i--] = 0;
        }
      }
      else
      {
        value[0] = 0;
      }
      set(name, value);
    }

    return result || check();
  }

  bool AdminCmd::SingleValueExaminer::examine (std::istream& in)
  {
    std::ostringstream out;
    char c;
    while (in.get(c))
    {
      out << c;
    }
    admin_.values_.push_back(values_type(1, out.str()));
    return admin_.check();
  }

  ShellCmd::DumpExaminer::DumpExaminer(
    Logger& logger_,
    unsigned long log_level_)
    : logger(logger_),
      log_level(log_level_)
  {}

  bool ShellCmd::DumpExaminer::examine (std::istream& in)
  {
    char line[4096];
    std::ostringstream ostr;
    ostr << std::endl;
    while(!in.eof())
    {
      in.getline(line, sizeof(line));
      ostr << "  " << line << std::endl;
    }
    logger.log(ostr.str(), log_level);         
    return true;
  }

  void ShellCmd::log(
    Logger& logger,
    unsigned long log_level,
    bool rethrow)
  {
    DumpExaminer examiner (logger, log_level);
    std::ostringstream ostr;
    print_idname(ostr);        
    logger.log(ostr.str(), log_level);
    try
    {
      ShellCmd::fetch(examiner);
    }
    catch (CmdStatusFailed& exc)
    {
      Stream::Error ostr;
      ostr << "Admin execution error: " << std::endl <<
        exc.what();
      logger.log(ostr.str(), log_level);
      if (rethrow)
        throw;
    }
  }

  bool AdminCmd::examine(BasicExaminer& examiner)
  {
    values_.clear();
    return ShellCmd::fetch(examiner);
  }

  void AdminCmd::setup (size_t count, const char* const* names)
  {
    names_.resize(count);
    for(size_t i = 0; i < count; ++i)
    {
      if ( !names[i] )
      {
        throw Exception("Incorrect admin fields initialization");
      }
      names_[i] = names[i];
    }
  }

  void AdminCmd::Examiner::set (const char* name, const char* value)
  {
    Logger::thlog().debug_trace(
      std::string("AdminCmd: try to handle: name = ") +
        name + "; value = " + value + ";");
    for(size_t i =0; i < admin_.slice_size(); ++i)
    {
      if(admin_.names_[i] == name)
      {
        values_[i] = value;
        return;
      }
    }
    Logger::thlog().debug_trace(
      std::string("AdminCmd: not found how to handle: name = ") +
        name + "; value = " + value + ";");
  }

  void AdminCmd::add (const values_type& values)
  {
    if(slice_size() == values.size())
    {
      values_.push_back(values);
    }
  }

  bool AdminCmd::Examiner::check ()
  {
    Logger::thlog().debug_trace(
      String::SubString("AdminCmd: going to check fetched values"));
    bool no_values = true;
    for(size_t i =0; i < admin_.slice_size(); ++i)
    {
      if(!values_[i].empty())
      {
        no_values = false;
        break;
      }
    }
    if(no_values)
    {
      Logger::thlog().debug_trace(
        String::SubString("AdminCmd: try to check with empty values"));
      return false;
    }
    admin_.add(values_);
    values_.clear();
    values_.resize(admin_.slice_size());
    return admin_.check();
  }

  bool AdminCmd::check (const Comparable* expects, size_t i)
  {
    if(empty()) return false;
    return expects->compare(values_.back()[i].c_str());
  }

  bool AdminCmd::check ()
  {
    bool all_found = true;
    for(size_t j =0; j < expects_.size(); ++j)
    {
      bool item_found = true;
      if(!finds_[j])
      {
        for(size_t i =0; i < slice_size(); ++i)
        {
          if(!check(expects_[j][i], i))
          {
            item_found = false;
            break;
          }
        }
        finds_[j] = item_found;
      }
      all_found = all_found && item_found;
    }
    return all_found;
  }  

  bool AdminCmd::setup_examine(
    BasicExaminer& examiner,
    size_t size,
    const std::string* nvpair)
  {
    names_.resize(size/2);
    for(size_t i = 0; i < size; i += 2)
    {
      names_[i/2] = nvpair[i];
    }
    finds_.resize(1);
    finds_[0] = false;
    expects_.resize(1);
    expects_[0].resize(size/2);
    for(size_t i = 1; i < size; i += 2)
    {
      expects_[0][(i-1)/2].set(ComparableRegExp(nvpair[i]));
    }
    return examine(examiner);
  }

  bool AdminCmd::examine(const char* expects)
  {
    finds_.resize(1);
    finds_[0] = false;
    expects_.resize(1);
    expects_[0].resize(1);
    expects_[0][0].set(ComparableRegExp(expects));
    names_.resize(1);
    names_[0] = "single value";
    SingleValueExaminer examiner(*this);
    return examine(examiner);
  }

  bool AdminCmd::examine(
    BasicExaminer& examiner,
    size_t size,
    const char* const* expects)
  {
    finds_.resize(1);
    finds_[0] = false;
    expects_.resize(1);
    expects_[0].resize(size);
    for(size_t i = 0; i < size; ++i)
    {
      expects_[0][i].set(ComparableRegExp(expects[i]));
    }
    return examine(examiner);
  }

  bool AdminCmd::examine(
    BasicExaminer& examiner,
    size_t size,
    const std::string expects[])
  {
    finds_.resize(1);
    finds_[0] = false;
    expects_.resize(1);
    expects_[0].resize(size);
    for(size_t i = 0; i < size; ++i)
    {
      expects_[0][i].set(ComparableRegExp(expects[i]));
    }
    return examine(examiner);
  }

  bool AdminCmd::examine(
    BasicExaminer& examiner,
    size_t sizex,
    size_t size,
    const std::string* expects)
  {
    finds_.resize(sizex);
    expects_.resize(sizex);
    for(size_t j = 0; j < sizex; ++j)
    {
      finds_[j] = false;
      expects_[j].resize(size);
      for(size_t i = 0; i < size; ++i)
      {
        expects_[j][i].set(ComparableRegExp(expects[j*size + i]));
      }
    }
    return examine(examiner);
  }

  bool AdminCmd::examine(
    BasicExaminer& examiner,
    size_t sizex,
    size_t size,
    const char* const* expects)
  {
    finds_.resize(sizex);
    expects_.resize(sizex);
    for(size_t j = 0; j < sizex; ++j)
    {
      finds_[j] = false;
      expects_[j].resize(size);
      for(size_t i = 0; i < size; ++i)
      {
        expects_[j][i].set(ComparableRegExp(expects[j*size + i]));
      }
    }
    return examine(examiner);
  }

  std::ostream& AdminCmd::dumpout (
    std::ostream& out,
    const values_list_type& values) const
  {
    out << '{' << std::endl;
    for(size_t j = 0; j < values.size(); ++j)
    {
      out << "  {" << std::endl;
      for(size_t i = 0; i < slice_size(); ++i)
      {
        out << "    " << names_[i] << " = " << values[j][i] << ';' << std::endl;
      }
      out << "  }" << std::endl;
    }
    out << '}' << std::endl;
    return out;
  }

  std::ostream& AdminCmd::dumpout (
    std::ostream& out,
    const expects_list_type& values) const
  {
    out << '{' << std::endl;
    for(size_t j = 0; j < values.size(); ++j)
    {
      out << "  {" << std::endl;
      for(size_t i = 0; i < slice_size(); ++i)
      {
        out << "    " << names_[i] << " = " << values[j][i]->str() << ';' << std::endl;
      }
      out << "  }" << std::endl;
    }
    out << '}' << std::endl;
    return out;
  }

  std::ostream& AdminCmd::dump_expects (std::ostream& out) const
  {
    out << "expect:" << std::endl;
    return dumpout(out, expects_);
  }

  std::ostream& AdminCmd::dumpout (std::ostream& out) const
  {
    return dumpout(out, values_);
  }

  bool AdminCmd::check_(
    size_t size,
    const char* const* names,
    const FieldIndexMap& expected_values)
    /*throw(eh::Exception)*/
  {
    finds_.resize(1);
    finds_[0] = false;
    // pack names
    names_.resize(size);
    for(size_t i = 0; i < size; ++i)
    {
      names_[i] = names[i];
    }
    // pack values
    expects_.resize(1);
    expects_[0].resize(size);
    for(size_t i = 0; i < size; ++i)
    {
      FieldIndexMap::const_iterator val_it = expected_values.find(i);
      if(val_it != expected_values.end())
      {
        expects_[0][i] = val_it->second;
      }
      else
      {
        expects_[0][i].set(ComparableRegExp(ANY));
      }
    }

    Examiner examiner(*this, false);
    return examine(examiner);
  }

  std::string
  get_field_list(const AdminCmd& adm, unsigned int field)
  {
    std::string result;
    if (adm.empty() || adm[0].size() <= field)
      return result;
    for (unsigned int i = 0; i < adm.size() - 1; ++i)
    {
      result += adm[i].at(field) + ",";
    }
    result += adm.last().at(field);
    return result;
  }
  
}

