#pragma once

#include <cstddef>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <eh/Errno.hpp>
#include <eh/Exception.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Stream/MemoryStream.hpp>
#include <String/SubString.hpp>
#include <String/TextTemplate.hpp>
#include <Commons/AsyncCache.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/FileDescriptorGuard.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace AdServer::Commons
{
  class TextTemplate: public String::TextTemplate::String
  {
  public:
    explicit
    TextTemplate(std::string_view str);

    explicit
    TextTemplate(const ::String::SubString& str);

    ~TextTemplate() noexcept = default;
  };

  using TextTemplatePtr = std::shared_ptr<TextTemplate>;

  using FileContent = std::string;
  using FileContentPtr = std::shared_ptr<FileContent>;

  template<typename Text>
  class TextTemplateAsyncCache:
    public AsyncCache<std::string, std::shared_ptr<Text>, std::string>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using Key = std::string;
    using Value = std::shared_ptr<Text>;
    using Base = AsyncCache<Key, Value, std::string>;
    using Holder = typename Base::Holder;
    using HolderPtr = typename Base::HolderPtr;
    using UpdateCallback = typename Base::UpdateCallback;
    using FarUpdateCallback =
      std::function<void(std::optional<std::string> data)>;
    using FarUpdateStrategy = std::function<void(
      std::string key,
      std::string service_id,
      FarUpdateCallback callback)>;

    TextTemplateAsyncCache(
      std::size_t max_size,
      Generics::TaskRunner* task_runner,
      const Generics::Time& max_keep_time,
      const Generics::Time& update_period = Generics::Time::ONE_SECOND,
      FarUpdateStrategy far_update_strategy = FarUpdateStrategy());

  private:
    static typename Base::SyncUpdate
    make_sync_update_();

    static typename Base::AsyncUpdate
    make_async_update_(
      Generics::TaskRunner* task_runner,
      FarUpdateStrategy far_update_strategy);

    static HolderPtr
    load_local_(
      std::string_view path,
      const HolderPtr& old_holder);

    static void
    call_update_callback_(
      const std::shared_ptr<UpdateCallback>& callback,
      const std::shared_ptr<std::atomic_bool>& callback_called,
      HolderPtr holder)
      noexcept;

  private:
    Generics::TaskRunner_var task_runner_;
  };

  using TextTemplateCache = TextTemplateAsyncCache<TextTemplate>;
  using TextTemplateCachePtr = std::shared_ptr<TextTemplateCache>;

  using FileCache = TextTemplateAsyncCache<FileContent>;
  using FileCachePtr = std::shared_ptr<FileCache>;
}

namespace AdServer::Commons
{
  inline
  TextTemplate::TextTemplate(std::string_view str)
    : String(
        ::String::SubString(str.data(), str.size()),
        ::String::SubString("##"),
        ::String::SubString("##"))
  {}

  inline
  TextTemplate::TextTemplate(const ::String::SubString& str)
    : TextTemplate(std::string_view(str.data(), str.size()))
  {}

  template<typename Text>
  inline
  TextTemplateAsyncCache<Text>::TextTemplateAsyncCache(
    std::size_t max_size,
    Generics::TaskRunner* task_runner,
    const Generics::Time& max_keep_time,
    const Generics::Time& update_period,
    FarUpdateStrategy far_update_strategy)
    : Base(
        max_size,
        update_period,
        max_keep_time,
        make_sync_update_(),
        make_async_update_(task_runner, std::move(far_update_strategy))),
      task_runner_(ReferenceCounting::add_ref(task_runner))
  {
    if(task_runner_.in() == 0)
    {
      throw Exception("TextTemplateAsyncCache: task_runner is null");
    }
  }

  template<typename Text>
  inline typename TextTemplateAsyncCache<Text>::Base::SyncUpdate
  TextTemplateAsyncCache<Text>::make_sync_update_()
  {
    return [](
      const std::string& path,
      const HolderPtr& old_holder,
      const std::string&) -> HolderPtr
    {
      return load_local_(path, old_holder);
    };
  }

  template<typename Text>
  inline typename TextTemplateAsyncCache<Text>::Base::AsyncUpdate
  TextTemplateAsyncCache<Text>::make_async_update_(
    Generics::TaskRunner* task_runner,
    FarUpdateStrategy far_update_strategy)
  {
    return [
      task_runner,
      far_update_strategy = std::move(far_update_strategy)
    ](
      std::string path,
      HolderPtr old_holder,
      UpdateCallback callback,
      std::string service_id)
    {
      auto callback_holder =
        std::make_shared<UpdateCallback>(std::move(callback));
      auto callback_called = std::make_shared<std::atomic_bool>(false);

      try
      {
        if(task_runner == 0)
        {
          call_update_callback_(
            callback_holder,
            callback_called,
            HolderPtr());
          return;
        }

        task_runner->enqueue_task(AdServer::Commons::make_delegate_task(
          [
            path = std::move(path),
            old_holder = std::move(old_holder),
            service_id = std::move(service_id),
            far_update_strategy,
            callback_holder,
            callback_called
          ]() mutable noexcept
          {
            try
            {
              struct stat fs;
              if (::stat(path.c_str(), &fs) < 0)
              {
                if (!far_update_strategy)
                {
                  call_update_callback_(
                    callback_holder,
                    callback_called,
                    HolderPtr());
                  return;
                }

                far_update_strategy(
                  std::move(path),
                  std::move(service_id),
                  [
                    callback_holder,
                    callback_called
                  ](std::optional<std::string> data) mutable noexcept
                  {
                    const Generics::Time now = Generics::Time::get_time_of_day();
                    if (!data)
                    {
                      call_update_callback_(
                        callback_holder,
                        callback_called,
                        std::make_shared<Holder>(
                          std::optional<Value>(),
                          now,
                          Generics::Time::ZERO,
                          0));
                      return;
                    }

                    const std::string_view view(data->data(), data->size());
                    call_update_callback_(
                      callback_holder,
                      callback_called,
                      std::make_shared<Holder>(
                        std::make_shared<Text>(view),
                        now,
                        now,
                        data->size()));
                  });
                return;
              }

              call_update_callback_(
                callback_holder,
                callback_called,
                load_local_(path, old_holder));
            }
            catch(...)
            {
              call_update_callback_(
                callback_holder,
                callback_called,
                HolderPtr());
            }
          }));
      }
      catch(...)
      {
        call_update_callback_(
          callback_holder,
          callback_called,
          HolderPtr());
      }
    };
  }

  template<typename Text>
  inline void
  TextTemplateAsyncCache<Text>::call_update_callback_(
    const std::shared_ptr<UpdateCallback>& callback,
    const std::shared_ptr<std::atomic_bool>& callback_called,
    HolderPtr holder)
    noexcept
  {
    bool expected = false;
    if(callback_called->compare_exchange_strong(expected, true))
    {
      try
      {
        if(callback && *callback)
        {
          (*callback)(std::move(holder));
        }
      }
      catch(...)
      {}
    }
  }

  template<typename Text>
  inline typename TextTemplateAsyncCache<Text>::HolderPtr
  TextTemplateAsyncCache<Text>::load_local_(
    std::string_view path,
    const HolderPtr& old_holder)
  {
    static const char* FUN = "TextTemplateAsyncCache<Text>::load_local_()";

    const Generics::Time now = Generics::Time::get_time_of_day();

    struct stat fs;
    const std::string path_string(path);
    if (::stat(path_string.c_str(), &fs) < 0)
    {
      eh::throw_errno_exception<Exception>(
        errno, FUN, ": failed to stat file '", path_string, "'");
    }

    const Generics::Time new_mod_time(fs.st_mtime);

    if (old_holder && old_holder->value && new_mod_time == old_holder->mod_time)
    {
      return std::make_shared<Holder>(
        *old_holder->value,
        now,
        new_mod_time,
        old_holder->size);
    }

    const int fd = ::open(path_string.c_str(), O_RDONLY);
    if (fd < 0)
    {
      eh::throw_errno_exception<Exception>(
        errno, FUN, ": failed to open file '", path_string, "'");
    }

    try
    {
      FileDescriptorGuard fd_guard{FileDescriptor(fd)};
      if (fs.st_size < 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": negative file size for '" << path_string << "'";
        throw Exception(ostr);
      }

      std::string file_data;
      file_data.resize(static_cast<std::size_t>(fs.st_size));
      std::size_t read_size = 0;
      while(read_size < file_data.size())
      {
        const ssize_t ret = ::read(
          fd_guard.get().fd,
          file_data.data() + read_size,
          file_data.size() - read_size);
        if (ret < 0)
        {
          if (errno == EINTR)
          {
            continue;
          }

          eh::throw_errno_exception<Exception>(
            errno, FUN, ": failed to read file '", path_string, "'");
        }

        if (ret == 0)
        {
          file_data.resize(read_size);
          break;
        }

        read_size += static_cast<std::size_t>(ret);
      }

      const std::string_view view(file_data.data(), file_data.size());

      return std::make_shared<Holder>(
        std::make_shared<Text>(view),
        now,
        new_mod_time,
        file_data.size());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
