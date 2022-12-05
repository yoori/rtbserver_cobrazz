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

namespace AdServer
{
  namespace Predictor
  {

    /**
     * Input PRImpression csv stream
     */
    class InputCsvStream
    {

      DECLARE_EXCEPTION(ParseError, eh::DescriptiveException);
      
    public:

      /**
       * @brief Constructor.
       *
       * @param file object
       * @param logger
       */
      InputCsvStream(
        const FileObject& file,
        Logging::Logger* logger);

      /**
       * @brief Parse csv header.
       *
       * @param[out]  feature column list
       * @return label column index
       */
      unsigned long
      parse_header_(
        std::istream& stream,
        FeatureColumns& feature_columns);
      
      /**
       * @brief Process input stream.
       *
       * @param libsvm output streams
       */
      bool process(
        OutputLibSvmStreamList& lib_svms);

      /**
       * @brief Destructor.
       */
      virtual ~InputCsvStream()
        noexcept;

    private:
      FileObject file_;
      Logging::Logger_var logger_;
    };

  }
}
