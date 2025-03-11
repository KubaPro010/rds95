#!/usr/bin/python


#   PiFmRds - FM/RDS transmitter for the Raspberry Pi
#   Copyright (C) 2014 Christophe Jacquet, F8FTK
#
#   See https://github.com/ChristopheJacquet/PiFmRds
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#   This program generates the waveform of a single biphase symbol
#
#   This program uses Pydemod, see https://github.com/ChristopheJacquet/Pydemod

import pydemod.app.rds as rds
import numpy
import io
import matplotlib.pyplot as plt

sample_rate = 9500

outc = io.open("waveforms.c", mode="w", encoding="utf8")
outh = io.open("waveforms.h", mode="w", encoding="utf8")

header = u"""
/* This file was automatically generated by "gen_wave.py".
   (C) 2014 Christophe Jacquet.
   Modified by kuba201
   Released under the GNU GPL v3 license.
*/

"""

outc.write(header)
outh.write(header)

def generate_bit(name):
    offset = int(sample_rate*0.004) # 190 khz = 760
    count = int(offset / 10**(len(str(offset)) - 1)) # 760 / 100 = 7
    l = int(sample_rate / 1187.5) // 2 # 16/2 = 8
    if l == 1: raise Exception("Sample rate too small")

    sample = numpy.zeros(count*l)
    sample[l] = 1
    sample[2*l] = -1

    # Apply the data-shaping filter
    sf = rds.pulse_shaping_filter(l*16, sample_rate-1)
    shapedSamples = numpy.convolve(sample, sf)

    out = shapedSamples[offset-l*count:offset+l*count] #[offset:offset+l*count]
    #plt.plot(sf)
    #plt.plot(shapedSamples)
    plt.plot(out)
    plt.show()

    outc.write(u"float waveform_{name}[{size}] = {{{values}}};\n\n".format(
        name = name,
        values = u", ".join(map(str, out / 2.5)),
        size = len(out)))
        # note: need to limit the amplitude so as not to saturate when the biphase
        # waveforms are summed

    outh.write(u"extern float waveform_{name}[{size}];\n".format(name=name, size=len(out)))


generate_bit("biphase")

outc.close()
outh.close()