# This file is part of Pydemod
# Copyright Christophe Jacquet (F8FTK), 2014
# Licence: GNU GPL v3
# See: https://github.com/ChristopheJacquet/Pydemod

import numpy

import pydemod.filters.shaping as shaping

def pulse_shaping_filter(length, sample_rate):
    return shaping.rrcosfilter(length, 1, 1/(2*1187.5), sample_rate+1) [1]