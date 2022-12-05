#!/usr/bin/env python

import os, sys

AdsCommon = os.path.abspath('../../../../build/tests/AutoTests/PyUtils/Cluster')
sys.path.insert(0, AdsCommon)
PyCommons = os.path.abspath('../../../../PyCommons')
sys.path.insert(0, PyCommons)
sys.path.insert(0, '..')
sys.path.insert(0, './CampaignManagerTest')

from funOrbRegressionTest import  funOrbRegressionTest
import os

def main():
  funOrbRegressionTest(os.getcwd())

if __name__ == '__main__':
  main()

