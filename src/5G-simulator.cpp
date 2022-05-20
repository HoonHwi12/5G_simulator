/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of 5G-air-simulator
 *
 * 5G-air-simulator is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * 5G-air-simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 5G-air-simulator; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Telematics Lab <telematics-dev@poliba.it>
 * Author: Sergio Martiradonna <sergio.martiradonna@poliba.it>
 */


/*
 * 5G-air-simulator is the main program of the simulator.
 * To run simulations you can
 * (i) select one of example scenarios developed in Scenario/ ("./5G-air-simulator -h" for details)
 * (ii) create a new scenario and add its reference into the main program.
 *
 *  To create a new scenario, see documentation in DOC/ folder.
 *
 *  For any questions, please contact the author at
 *  g.piro@poliba.it
 */


#include "scenarios/simple.h"
#include "scenarios/single-cell-without-interference.h"
#include "scenarios/single-cell-with-interference.h"
#include "scenarios/single-cell-with-femto.h"
#include "scenarios/multi-cell.h"
#include "scenarios/single-cell-with-streets.h"
#include "scenarios/multi-cell-sinrplot.h"
#include "scenarios/itu-calibration.h"
#include "scenarios/urban-macrocell-itu.h"
#include "scenarios/rural-macrocell-itu.h"
#include "scenarios/f5g-uc1.h"
#include "scenarios/f5g-uc2.h"
#include "scenarios/f5g-uc6.h"
#include "scenarios/MMC1.h"
#include "scenarios/nb-cell.h"
#include "scenarios/test-demo1.h"
#include "scenarios/test-tri-sector.h"
#include "scenarios/test-multi-cell-tri-sector.h"
#include "scenarios/test-multicast.h"
#include "scenarios/test-mbsfn.h"
#include "scenarios/test-unicast.h"
#include "scenarios/nb-cell-test.h"

#include "utility/help.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <fenv.h>

//by HH
#include "scenarios/single-cell-with-interference-mixed-apps.h"

std::mt19937 commonGen(time(NULL));

int
main (int argc, char *argv[])
{
  // Raise a floating point error when some computation gives a NaN as result
//  feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT);

  if (argc > 1)
    {
      /* Help */
      if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-H") || !strcmp(argv[1],
          "--help") || !strcmp(argv[1], "--Help"))
        {
          Help ();
          return 0;
        }

      /* Run simple scenario */
      else if (strcmp(argv[1], "Simple")==0)
        {
          Simple ();
        }


      /* Run more complex scenarios */
      else if (strcmp(argv[1], "SingleCell")==0)
        {
          SingleCellWithoutInterference (argc, argv);
        }
      else if (strcmp(argv[1], "SingleCellWithI")==0)
        {
          SingleCellWithInterference (argc, argv);
        }
      else if (strcmp(argv[1], "MultiCell")==0)
        {
          MultiCell (argc, argv);
        }
      else if (strcmp(argv[1], "SingleCellWithFemto")==0)
        {
          SingleCellWithFemto(argc, argv);
        }
      else if (strcmp(argv[1], "SingleCellWithStreets")==0)
        {
          SingleCellWithStreets (argc, argv);
        }
      else if (strcmp(argv[1], "MMC1")==0)
        {
          MMC1 (argc, argv);
        }
      else if (strcmp(argv[1], "nbCell")==0)
        {
          nbCell (argc, argv);
        }
      else if (strcmp(argv[1], "test-tri-sector")==0)
        {
          TestTriSector (argc, argv);
        }
      else if (strcmp(argv[1], "test-multi-cell-tri-sector")==0)
        {
          TestMultiCellTriSector (argc, argv);
        }
      else if (strcmp(argv[1], "itu-calibration")==0)
        {
          ItuCalibration (argc, argv);
        }
      else if (strcmp(argv[1], "urban-macrocell-itu")==0)
        {
          UrbanMacrocellItu (argc, argv);
        }
      else if (strcmp(argv[1], "rural-macrocell-itu")==0)
        {
          RuralMacrocellItu (argc, argv);
        }
      else if (strcmp(argv[1], "test-multicast")==0)
        {
          TestMulticast (argc, argv);
        }
      else if (strcmp(argv[1], "test-mbsfn")==0)
        {
          TestMbsfn (argc, argv);
        }
      else if (strcmp(argv[1], "test-unicast")==0)
        {
          TestUnicast (argc, argv);
        }
      else if (strcmp(argv[1], "f5g-uc1")==0)
        {
          f5g_50MbpsEverywhere (argc, argv);
        }
      else if (strcmp(argv[1], "f5g-uc2")==0)
        {
          f5g_HighSpeedTrain (argc, argv);
        }
      else if (strcmp(argv[1], "f5g-uc6")==0)
        {
          f5g_BroadcastServices (argc, argv);
        }
      else if (strcmp(argv[1], "f5g-demo1")==0)
        {
          f5g_demo1 (argc, argv);
        }
      else if (strcmp(argv[1], "nbCellTest")==0)
        {
          nbCellTest (argc, argv);
        }
      else if (strcmp(argv[1], "SingleCellWithIMixedApps")==0)
	    {
        int nbCells = atoi(argv[2]);
        double radius = atof(argv[3]);
        int nbUE = atoi(argv[4]);
        int nbVideo = atoi(argv[5]);
        int nbCBR = atoi(argv[6]);
        int nbBE = atoi(argv[7]);
        int nbVOIP = atoi(argv[8]);
        int sched_type = atoi(argv[9]);
        int frame_struct = atoi(argv[10]);
        int speed = atoi(argv[11]);
        double maxDelay = atof(argv[12]);
        int video_bit_rate = atoi(argv[13]);
        int seed;
        if (argc==15) seed = atoi(argv[14]);
        else seed = -1;
        SingleCellWithInterferenceMixedApps(nbCells, radius, nbUE, nbVideo, nbCBR, nbBE, nbVOIP, sched_type, frame_struct, speed, maxDelay, video_bit_rate, seed);
	    }
      else
      {
        printf("No test selected: ABORT\n");
      }
    }
  else
    {
      Help ();
    }

    return 0;
}
