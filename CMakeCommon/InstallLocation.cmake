#==================================================================
# CHECK THE OS AND SCHOOL AND FIGURE WHERE TO INSTALL
#
# this code figures out where this compile is running in
# order to apply some rules
# about where to install the results
#==================================================================
IF(WIN32)
   MESSAGE(STATUS "Windows system detected....")
   EXEC_PROGRAM("hostname" OUTPUT_VARIABLE HOSTNAME)
   EXEC_PROGRAM("nslookup" ARGS ${HOSTNAME} OUTPUT_VARIABLE DOMAIN)
   MESSAGE(STATUS "Domain is " ${DOMAIN})
   IF(DOMAIN MATCHES cim)
      MESSAGE(STATUS "CIM Detected")
      SET(CMAKE_INSTALL_PREFIX "I:/tools/windowscustom/" CACHE FILEPATH "Install path prefix, prepended onto install directories.")
   ENDIF(DOMAIN MATCHES cim)
   IF(DOMAIN MATCHES bic)
      MESSAGE(STATUS "BIC detected")
      SET(CMAKE_INSTALL_PREFIX "C:/Program Files/mni/")
   ENDIF(DOMAIN MATCHES bic)
ELSE(WIN32)
   IF(UNIX)
   MESSAGE(STATUS "Unix system detected....")
   EXEC_PROGRAM(domainname OUTPUT_VARIABLE DOMAIN)
   EXEC_PROGRAM(dnsdomainname OUTPUT_VARIABLE DNSDOMAIN)
   MESSAGE(STATUS ${DOMAIN})
   IF(DOMAIN MATCHES CIM)
      MESSAGE( STATUS "CIM detected" )
      SET(CMAKE_INSTALL_PREFIX "/home/rain/igns/tools/custom/"  CACHE FILEPATH "Install path prefix, prepended onto install directories.")
   ENDIF(DOMAIN MATCHES CIM)
#   IF( DNSDOMAIN MATCHES "bic.mni.mcgill.ca" )
#      MESSAGE( STATUS "BIC detected" )
#      SET(CMAKE_INSTALL_PREFIX "/usr/local/Igns/")
#   ENDIF( DNSDOMAIN MATCHES "bic.mni.mcgill.ca" )
   ENDIF(UNIX)
ENDIF(WIN32)

MESSAGE(STATUS "CMAKE_INSTALL_PREFIX will be " ${CMAKE_INSTALL_PREFIX})
