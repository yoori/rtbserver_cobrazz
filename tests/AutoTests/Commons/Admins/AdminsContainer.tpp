namespace AutoTest
{
  template<typename AdminType, CheckType ch>
  AdminsArray<AdminType, ch>::AdminsArray(
    const AdminsArray<AdminType, ch>& from)
    : admins_(from.admins_)
  {}

  template<typename AdminType, CheckType ch>
  void
  AdminsArray<AdminType, ch>::initialize(
    BaseUnit* test,
    ClusterTypeEnum cluster,
    ServiceTypeEnum service)
  {
    clear();

    const ServiceList& services =
      test->get_config().get_services(
        cluster, service);

    for (Service_iterator it = services.begin(); it != services.end(); ++it)
    {
      add(AdminType(it->address.c_str()));
    }
  }

  template<typename AdminType, CheckType ch>
  template<typename Arg>
  void
  AdminsArray<AdminType, ch>::initialize(
    BaseUnit* test,
    ClusterTypeEnum cluster,
    ServiceTypeEnum service,
    Arg arg)
  {
    clear();
    
    const ServiceList& services =
      test->get_config().get_services(
        cluster, service);

    for (Service_iterator it = services.begin(); it != services.end(); ++it)
    {
      add(AdminType(it->address.c_str(), arg));
    }
  }

  template<typename AdminType, CheckType ch>
  template<typename Arg1, typename Arg2>
  void
  AdminsArray<AdminType, ch>::initialize(
    BaseUnit* test,
    ClusterTypeEnum cluster,
    ServiceTypeEnum service,
    Arg1 arg1,
    Arg2 arg2)
  {
    clear();
    
    const ServiceList& services =
      test->get_config().get_services(
        cluster, service);

    for (Service_iterator it = services.begin(); it != services.end(); ++it)
    {
      add(AdminType(it->address.c_str(), arg1, arg2));
    }
  }

  template<typename AdminType, CheckType ch>
  template<typename Arg1, typename Arg2, typename Arg3>
  void
  AdminsArray<AdminType, ch>::initialize(
    BaseUnit* test,
    ClusterTypeEnum cluster,
    ServiceTypeEnum service,
    Arg1 arg1,
    Arg2 arg2,
    Arg3 arg3)
  {
    clear();
    
    const ServiceList& services =
      test->get_config().get_services(
        cluster, service);

    for (Service_iterator it = services.begin(); it != services.end(); ++it)
    {
      add(AdminType(it->address.c_str(), arg1, arg2, arg3));
    }
  }

  template<typename AdminType, CheckType ch>
  void AdminsArray<AdminType, ch>::add(const AdminType& admin)
  {
    admins_.push_back(admin);
  }

  template<typename AdminType, CheckType ch>
  void AdminsArray<AdminType, ch>::del()
  {
    if (!admins_.empty())
      admins_.pop_back();
  }

  template<typename AdminType, CheckType ch>
  void AdminsArray<AdminType, ch>::clear()
  {
    if (!admins_.empty())
      admins_.clear();
  }

  template<typename AdminType, CheckType ch>
  AdminType& AdminsArray<AdminType, ch>::operator [] (int i)
  {
    return admins_.at(i);
  }

  template<typename AdminType, CheckType ch>
  const AdminType& AdminsArray<AdminType, ch>::operator [] (int i) const
  {
    return admins_.at(i);
  }

  template<typename AdminType, CheckType ch>
  template<typename Expects>
  bool AdminsArray<AdminType, ch>::check(
    const Expects& expects,
    bool exists)
  {
    unsigned int succed = 0;
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      if (exists == it->check(expects))
      {
          ++succed;
      }
    }
    return
      (!exists && succed == admins_.size() && ch != CT_ONE_NOT_EXPECTED) ||
      (ch == CT_ANY && succed && exists) ||
      (ch == CT_ONE && succed == 1  && exists) ||
      (ch == CT_ALL && succed == admins_.size() && exists) ||
      (ch == CT_ONE_NOT_EXPECTED && !exists && succed == 1) ||
      (ch == CT_ONE_NOT_EXPECTED && exists && succed == admins_.size());
  }

  template<typename AdminType, CheckType ch>
  bool AdminsArray<AdminType, ch>::fetch()
  {
    bool result = true;
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      result &= it->fetch();
    }
    return result;
  }

  template<typename AdminType, CheckType ch>
  void AdminsArray<AdminType, ch>::exec()
  {
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      it->exec();
    }
  }

  template<typename AdminType, CheckType ch>
  void AdminsArray<AdminType, ch>::log(
    Logger& logger,
    unsigned long log_level)
  {
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      it->log(logger, log_level);
    }
  }
  
  template<typename AdminType, CheckType ch>
  bool AdminsArray<AdminType, ch>::empty() const
  {
    return admins_.empty();
  }

  template<typename AdminType, CheckType ch>
  std::ostream& AdminsArray<AdminType, ch>::print_idname(std::ostream& out)
  {
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      it->print_idname(out);
      out << "; ";
    }
    return out;
  }

  template<typename AdminType, CheckType ch>
  std::ostream& AdminsArray<AdminType, ch>::dump_expects(std::ostream& out)
  {
    if (!admins_.empty())
      return admins_.at(0).dump_expects(out);
    return out;
  }

  template<typename AdminType, CheckType ch>
  std::ostream& AdminsArray<AdminType, ch>::dumpout(std::ostream& out)
  {
    for (typename ArrayType::iterator it = admins_.begin();
         it != admins_.end();
         ++it)
    {
      out << "For command '";
      it->print_idname(out);
      out << "'" << std::endl;
      it->dumpout(out);
      out << std::endl;
    }
    return out;
  }
} // namespace AutoTest
