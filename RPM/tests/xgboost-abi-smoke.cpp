#include <xgboost/c_api.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

extern "C" bool check_filesystem();

void check(int result)
{
  if (result != 0)
  {
    std::fprintf(stderr, "%s\n", XGBGetLastError());
    std::exit(EXIT_FAILURE);
  }
}

int main()
{
  if (!check_filesystem())
  {
    return EXIT_FAILURE;
  }

  const float values[] = {0, 1, 1, 0, 2, 1, 3, 0};
  const float labels[] = {0, 1, 2, 3};
  DMatrixHandle matrix = nullptr;
  BoosterHandle booster = nullptr;
  check(XGDMatrixCreateFromMat(values, 4, 2, NAN, &matrix));
  check(XGDMatrixSetFloatInfo(matrix, "label", labels, 4));
  check(XGBoosterCreate(&matrix, 1, &booster));
  check(XGBoosterSetParam(booster, "nthread", "1"));
  check(XGBoosterSetParam(booster, "max_depth", "2"));
  check(XGBoosterUpdateOneIter(booster, 0, matrix));
  bst_ulong count = 0;
  const float* predictions = nullptr;
  check(XGBoosterPredict(booster, matrix, 0, 0, 0, &count, &predictions));
  if (count != 4)
  {
    return EXIT_FAILURE;
  }

  for (bst_ulong i = 0; i < count; ++i)
  {
    if (!std::isfinite(predictions[i]))
    {
      return EXIT_FAILURE;
    }
  }

  bst_ulong config_size = 0;
  const char* config = nullptr;
  check(XGBoosterSaveJsonConfig(booster, &config_size, &config));
  check(XGBoosterLoadJsonConfig(booster, config));
  check(XGBoosterFree(booster));
  check(XGDMatrixFree(matrix));
  std::puts("XGBoost ABI smoke: filesystem, initialization, training and prediction passed");
}
