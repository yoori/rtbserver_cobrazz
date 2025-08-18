/* $Id: InputCsvStream.hpp 185976 2020-07-01 22:11:23Z jurij_kuznecov $
* @file InputStream.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Input csv file stream
*/
#pragma once

#include "CampaignFolder.hpp"
#include "FileGuard.hpp"
#include "FeatureContainer.hpp"
#include "OutputLibSvmStream.hpp"

namespace AdServer::Predictor
{
  /**
   * Input PRImpression csv stream
   */
  class InputCsvStream
  {
  public:
    DECLARE_EXCEPTION(ParseError, eh::DescriptiveException);

  public:
    /**
     * @brief Constructor.
     *
     * @param file object
     * @param logger
     */
    InputCsvStream(const FileObject& file, Logging::Logger* logger);

    /**
     * @brief Destructor.
     */
    virtual ~InputCsvStream() noexcept;

    /**
     * @brief Process input stream.
     *
     * @param libsvm output streams
     */
    bool process(OutputLibSvm& lib_svm);

    static void
    process(
      OutputLibSvm& lib_svm,
      std::istream& stream,
      Logging::Logger* logger,
      const char* file_path);

  private:
    /**
     * @brief Parse csv header.
     *
     * @param[out]  feature column list
     * @return label column index
     */
    static unsigned long
    parse_header_(
      std::istream& stream,
      FeatureColumns& feature_columns);

  private:
    FileObject file_;
    Logging::Logger_var logger_;
  };
}
