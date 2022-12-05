
#include "BenchmarkStorage.hpp"

// class RequestStorage

RequestStorage::RequestStorage() :
  current_request_(requests_.begin())
{
}

RequestStorage::~RequestStorage() noexcept
{
}

void RequestStorage::push(const char* request)
{
  requests_.push_back(request);
  current_request_ = requests_.begin();
}

bool RequestStorage::get(std::string& request)
{
  assert(current_request_ != requests_.end());
  assert(requests_.size());
  assert(!(*current_request_).empty());
  request = *current_request_;
  if (++current_request_ == requests_.end())
  {
    current_request_ = requests_.begin();
    return true;
  }
  return false;
}

size_t RequestStorage::size()
{
  return requests_.size();
}

void RequestStorage::clear()
{
  requests_.clear();
  current_request_ = requests_.begin();
}


// class ClientStorage

ClientStorage::ClientStorage() :
  current_client_(requests_.begin()),
  size_(0)
{
}

ClientStorage::~ClientStorage()  noexcept
{
}

void ClientStorage::push_request(unsigned long client_id,
                                 const char* request)
{
  ClientIterator client = requests_.find(client_id);
  size_++;
  if (client == requests_.end())
  {
    requests_[client_id] =  RequestStorage_var(new RequestStorage());
    requests_[client_id]->push(request);
    current_client_ = requests_.begin();
    return;
  }
  client->second->push(request);
}

size_t ClientStorage::requests()
{
  return size_;
}

unsigned long ClientStorage::get_request(std::string& request)
{
  unsigned long client_id = current_client_->first;
  if (current_client_->second->get(request))
  {
    ++current_client_;
  }
  if ( current_client_ == requests_.end())
  {
    current_client_ = requests_.begin();
  }
  return client_id;
}

void ClientStorage::clear()
{
  requests_.clear();
  current_client_ = requests_.begin();
}

// class BenchmarkStorage

BenchmarkStorage::BenchmarkStorage(Statistics* stats) :
  storages_(REQUEST_TYPE_SIZE),
  current_client_(clients_.begin()),
  stats_(stats)
{
  for (unsigned long i=0; i < storages_.size(); ++i)
  {
    storages_[i] = ClientStorage_var(new ClientStorage());
  }
}

BenchmarkStorage::~BenchmarkStorage()  noexcept
{
}

void BenchmarkStorage::push_request_pair(FrontendType frontend_type,
                                         AdServerClientBase* client,
                                         const char* request) noexcept
{
  assert(frontend_type < REQUEST_TYPE_SIZE && frontend_type >= 0);
  WriteGuard_ guard(lock_);
  ClientStorage_var& client_storage = storages_[frontend_type];
  client_storage->push_request(client->id(), request);
}

void BenchmarkStorage::push_client(AdServerClientBase* client)
{
  WriteGuard_ guard(lock_);
  unsigned long client_id = client->id();
  ClientIterator client_it = clients_.find(client_id);
  if (client_it == clients_.end())
  {
    clients_[client_id] = AdServerClient_var(client);
    current_client_ = clients_.begin();
  }
}

AdServerClientBase*
BenchmarkStorage::get_request_pair(FrontendType frontend_type,
                                   std::string& request)
{
  assert(frontend_type < REQUEST_TYPE_SIZE && frontend_type >= 0);
  WriteGuard_ guard(lock_);
  ClientStorage_var& client_storage = storages_[frontend_type];
  unsigned long  client_id =  client_storage->get_request(request);
  ClientMap::iterator client = clients_.find(client_id);
  assert(client != clients_.end());
  return client->second;
}

AdServerClientBase*
BenchmarkStorage::get_client()
{
  WriteGuard_ guard(lock_);
  AdServerClientBase* client = current_client_->second;
  if (++current_client_ == clients_.end())
  {
    current_client_ = clients_.begin();
  }
  return client;
}

size_t BenchmarkStorage::request_size(FrontendType frontend_type)
{
  assert(frontend_type < REQUEST_TYPE_SIZE && frontend_type >= 0);
  ReadGuard_ guard(lock_);
  ClientStorage_var& client_storage = storages_[frontend_type];
  return client_storage->requests();
}

size_t BenchmarkStorage::client_size()
{
  ReadGuard_ guard(lock_);
  return clients_.size();
}

void BenchmarkStorage::clear()
{
  WriteGuard_ guard(lock_);
  clients_.clear();
  current_client_ = clients_.begin();
  for (unsigned long i=0; i < storages_.size(); ++i)
  {
    storages_[i]->clear();
  }
}

Statistics* BenchmarkStorage::stats()
{
  ReadGuard_ guard(lock_);
  return stats_;
}
