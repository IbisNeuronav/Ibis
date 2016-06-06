/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#ifndef TAG_IGNSMSG_H
#define TAG_IGNSMSG_H

//miscellaneous, mostly used in tracker settings
#define IGNS_MSG_ERROR                   "Error"
#define IGNS_MSG_INFO                    "Info"
#define IGNS_MSG_LOAD_DEFAULT_CFG        "Loading default hardware configuration."
#define IGNS_MSG_LOAD_USER_CFG           "Loading user hardware configuration."
#define IGNS_MSG_NO_CONFIG_FILE          "Settings file not found. Select User Settings."
#define IGNS_MSG_MAKE_CONFIG             "Settings file not provided. Configure the application manually."
#define IGNS_MSG_OPEN                    "open"
#define IGNS_MSG_OPEN_CFG_FILE           "Open cofiguration file"
#define IGNS_MSG_RESTORE_DEFAULT         ". Restoring default settings."

// Acquisition settings
#define IGNS_MSG_ACQ_EXIST               "Please select different directory.\nAcquisition data already saved in: "

// Minc reader
#define IGNS_MSG_MINC_CORRUPTED          "The MINC file may contain unsupported features and cannot be read."

//user permissions
#define IGNS_MSG_FILE_NOT_EXISTS         "File does not exist:\n"
#define IGNS_MSG_NO_WRITE                "No write permission:\n"
#define IGNS_MSG_NO_READ                 "No read permission:\n"
#define IGNS_MSG_CANT_MAKE_DIR           "Can't create directory:\n"

//limits
#define IGNS_MSG_POSITIVE                 "This must be a positive number."
#define IGNS_MSG_POSITIVE_LIMITED         "Setting default. This must be a positive not bigger than "
#define IGNS_MSG_POSITIVE_OR_ZERO         "This value may not be negative "

#endif //TAG_IGNSMSG_H
