/*=========================================================================

  Program:   Igns global tools
  Module:    $RCSfile: vtkMINC2.h,v $
  Language:  C++
  Date:      $Date: 2010-02-18 19:23:21 $
  Version:   $Revision: 1.3 $

  Copyright (c) Anka, IPL, BIC, MNI, McGill 
  All rights reserved.
  
=========================================================================*/

/* 
   This file contains additional variables not existing in "minc.h" that was
   distributed with minc version 1.4 and 2.0
   The choice of the name was unfortunate as it suggests usage with minc 2 while the number 2 was used to distinguish from vtkMINC.h used in vtk 5.4.
*/
#ifndef vtkMINC2_h
#define vtkMINC2_h

#include <string>
#include <map>

/* timestamp of acquired frametaken at start*/
#define MItime_stamp                "time_stamp"

/* The study  attributes  used in ibis */
#define MIapplication               "application"
#define MIapplication_version       "application_version"
#define MItracker                   "tracker"
#define MItracker_model             "tracker_model"
#define MItracker_version           "tracker_version"
#define MItransducer_model          "transducer_model"
#define MItransducer_rom            "transducer_rom"
#define MItransducer_calibration    "transducer_calibration"
#define MIpointer_model             "pointer_model"
#define MIpointer_rom               "pointer_rom"
#define MIpointer_calibration       "pointer_calibration"
#define MIreference_model           "reference_model"
#define MIreference_rom             "reference_rom"
#define MIcalibration_method        "calibration_method"
#define MIcalibration_version       "calibration_version"
#define MIcalibration_date          "calibration_date"
#define MIcalibration_time          "calibration_time"
#define MIfield_value               "field_value"
#define MIperforming_physician      "performing_physician"
#define MIserial_no                 "serial_no"
#define MIsoftware_version          "software_version"
#define MIstart_date                "start_date"

/* The acquisition and its attributes */
#define MIultrasound_depth              "ultrasound_depth"
#define MIultrasound_scale              "ultrasound_scale"
#define MIultrasound_acquisition_type   "ultrasound_acquisition_type"
#define MIultrasound_acquisition_color  "ultrasound_acquisition_color"
#define MISAR                           "SAR"
#define MIacquisition_time              "acquisition_time"
#define MIdelay_in_TR                   "delay_in_TR"
#define MIecho_number                   "echo_number"
#define MIecho_train_length             "echo_train_length"
#define MIflip_angle                    "flip_angle"
#define MIimage_time                    "image_time"
#define MIimage_type                    "image_type"
#define MImr_acq_type                   "mr_acq_type"
#define MInum_dyn_scans                 "num_dyn_scans"
#define MInum_phase_enc_steps           "num_phase_enc_steps"
#define MInum_slices                    "num_slices"
#define MIpercent_phase_fov             "percent_phase_fov"
#define MIpercent_sampling              "percent_sampling"
#define MIphase_enc_dir                 "phase_enc_dir"
#define MIpixel_bandwidth               "pixel_bandwidth"
#define MIreceive_coil                  "receive_coil"
#define MIseries_description            "series_description"
#define MIseries_time                   "series_time"
#define MIslice_order                   "slice_order"
#define MIslice_thickness               "slice_thickness"
#define MIstart_time                    "start_time"
#define MItransmit_coil                 "transmit_coil"
#define MIwindow_center                 "window_center"
#define MIwindow_width                  "window_width"
#define MIacquisition_id                "acquisition_id"

/* The patient and its attributes */
#define MIposition                      "position"

// The following map is used to assign study attributes to their values
typedef std::map<const char*,std::string> STUDY_ATTRIBUTES;

// The following map is used to assign acquisition attributes to their values
typedef std::map<const char*,std::string> ACQUISITION_ATTRIBUTES;

// The following map is used to assign patient attributes that can be defined in ibis to their values
typedef std::map<const char*,std::string> PATIENT_ATTRIBUTES;

/* End ifndef vtkMINC2_h */
#endif
