package AdServer::Predictor::VTRPredictModelGenerator;

use strict;
use AdServer::Predictor::CTRPredictModelGenerator;

sub start
{
  my ($host, $descr) = @_;
  return AdServer::Predictor::CTRPredictModelGenerator::start_objective(
    $host, $descr, 'VTR', 'vtr');
}

sub stop
{
  my ($host, $descr) = @_;
  return AdServer::Predictor::CTRPredictModelGenerator::stop_objective(
    $host, $descr, 'VTR');
}

sub is_alive
{
  my ($host, $descr) = @_;
  return AdServer::Predictor::CTRPredictModelGenerator::is_alive_objective(
    $host, $descr, 'VTR');
}

1;
