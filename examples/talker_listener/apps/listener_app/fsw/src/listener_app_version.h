/**
 * @file
 *
 *  The Listener App header file containing version information
 */

#ifndef LISTENER_APP_VERSION_H
#define LISTENER_APP_VERSION_H

/* Development Build Macro Definitions */

#define LISTENER_APP_BUILD_NUMBER    46 /*!< Development Build: Number of commits since baseline */
#define LISTENER_APP_BUILD_BASELINE  "equuleus-rc1" /*!< Development Build: git tag that is the base for the current development */
#define LISTENER_APP_BUILD_DEV_CYCLE "equuleus-rc2" /**< @brief Development: Release name for current development cycle */
#define LISTENER_APP_BUILD_CODENAME  "Equuleus" /**< @brief: Development: Code name for the current build */

/*
 * Version Macros, see \ref cfsversions for definitions.
 */
#define LISTENER_APP_MAJOR_VERSION 1  /*!< @brief Major version number. */
#define LISTENER_APP_MINOR_VERSION 1  /*!< @brief Minor version number. */
#define LISTENER_APP_REVISION      0  /*!< @brief Revision version number. Value of 0 indicates a development version.*/

/**
 * @brief Last official release.
 */
#define LISTENER_APP_LAST_OFFICIAL "v1.1.0"

/*!
 * @brief Mission revision.
 *
 * Reserved for mission use to denote patches/customizations as needed.
 * Values 1-254 are reserved for mission use to denote patches/customizations as needed. NOTE: Reserving 0 and 0xFF for
 * cFS open-source development use (pending resolution of nasa/cFS#440)
 */
#define LISTENER_APP_MISSION_REV 0xFF

#define LISTENER_APP_STR_HELPER(x) #x /*!< @brief Helper function to concatenate strings from integer macros */
#define LISTENER_APP_STR(x) \
    LISTENER_APP_STR_HELPER(x) /*!< @brief Helper function to concatenate strings from integer macros */

/*! @brief Development Build Version Number.
 * @details Baseline git tag + Number of commits since baseline. @n
 * See @ref cfsversions for format differences between development and release versions.
 */
#define LISTENER_APP_VERSION LISTENER_APP_BUILD_BASELINE "+dev" LISTENER_APP_STR(LISTENER_APP_BUILD_NUMBER)

/**
 * @brief Max Version String length.
 *
 * Maximum length that an OSAL version string can be.
 *
 */
#define LISTENER_APP_CFG_MAX_VERSION_STR_LEN 256

#endif /* LISTENER_APP_VERSION_H */
