#pragma once

#include "HemisphereApplet.h"

using namespace HS;

// hacks to effectively rewrite part of the applet boilerplate,
// making names and icons static
#define applet_name applet_name() final { return applet_name_(); } \
  static constexpr const char* applet_name_

#define applet_icon applet_icon() final { return applet_icon_(); } \
  static constexpr const uint8_t* applet_icon_

#include "ADSREG.h"
#include "ADEG.h"
#include "ASR.h"
#include "AttenuateOffset.h"
#ifdef PEWPEWPEW
#include "Binary.h"
#endif
#include "BootsNCat.h"
#include "Brancher.h"
#include "BugCrack.h"
#include "Burst.h"
#include "Button.h"
#include "BitBeat.h"
#include "Cumulus.h"
#include "CVRecV2.h"
#include "Calculate.h"
#include "TruthCat3.h"
#include "TruthCat4.h"
#include "Calibr8.h"
#include "Carpeggio.h"
#ifdef PEWPEWPEW
#include "Chordinator.h"
#endif
#include "ClockDivider.h"
#include "ClkToGate.h"
#ifdef ARDUINO_TEENSY41
#include "ClockSetupT4.h"
#else
#include "ClockSetup.h"
#endif
#include "ClockSkip.h"
#include "Combin8.h"
#include "Compare.h"
#include "CVSeq.h"
#include "DivSeq.h"
#include "DivSeq10.h"
#include "DrumMap.h"
#include "DualQuant.h"
#ifdef PEWPEWPEW
#include "OffsetQuant.h"
#endif
#include "TwoRings.h"
#if !defined(CUSTOM_BUILD) || defined(PEWPEWPEW)
#include "DuoTET.h"
#endif
#include "EbbAndLfo.h"
#ifdef ENABLE_APP_ENIGMA
#include "EnigmaJr.h"
#endif
#ifdef PEWPEWPEW
#include "EnsOscKey.h"
#endif
#include "EnvFollow.h"
#include "EnvSeq.h"
#include "EuclidO.h"
#include "EuclidX.h"
#ifdef PEWPEWPEW
#include "GameOfLife.h"
#endif
#include "GateDelay.h"
#include "GatedVCA.h"
#include "DrLoFi.h"
#include "Logic.h"
#include "LowerRenz.h"
#include "Metronome.h"
#ifdef __IMXRT1062__
#include "MidiLoop.h"
#endif
#ifdef PEWPEWPEW
#include "MultiScale.h"
#endif
#include "Palimpsest.h"
#include "Pigeons.h"
#include "PolyDiv.h"
#include "Ponglet.h"
#include "ProbabilityDivider.h"
#include "ProbabilityMelody.h"
#include "Relabi.h"
#include "ResetClock.h"
#include "RndWalk.h"
#ifdef PEWPEWPEW
#include "RunglBook.h"
#endif
#include "ScaleDuet.h"
#include "Schmitt.h"
#include "Scope.h"
#include "SequenceX.h"
#include "Seq32.h"
#include "SeqPlay7.h"
#include "ShiftGate.h"
#ifdef PEWPEWPEW
#include "ShiftReg.h"
#endif
#include "Shredder.h"
#include "Shuffle.h"
#include "Slew.h"
#include "Squanch.h"
#include "Stairs.h"
#include "Strum.h"
#include "Switch.h"
#include "SwitchSeq.h"
#include "TB3PO.h"
#include "TLNeuron.h"
#ifdef PEWPEWPEW
#include "Trending.h"
#endif
#include "TrigSeq.h"
#include "TrigSeq16.h"
#include "Tuner.h"
#include "VectorEG.h"
#include "VectorLFO.h"
#include "VectorMod.h"
#include "VectorMorph.h"
#include "Voltage.h"
#include "MarkoV.h"
#include "MarkovPerc.h"
#ifdef PEWPEWPEW
#include "WTVCO.h"
#endif
#include "Xfader.h"
#include "hMIDIIn.h"
#include "hMIDIOut.h"

#undef applet_name
#undef applet_icon

#include "AppletRegistry.h"

constexpr Registry reg = Registry<HemisphereApplet, HS::APPLET_SLOTS
#ifndef DISABLE_APPLET_ADSREG
    , DeclareApplet<ADSREG, 8, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_ADEG
    , DeclareApplet<ADEG, 34, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_MiniASR
    , DeclareApplet<MiniASR, 47, CAT_MODULATOR | CAT_QUANTIZER>
#endif
#ifndef DISABLE_APPLET_AttenuateOffset
    , DeclareApplet<AttenuateOffset, 56, CAT_UTILITY>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_Binary
    , DeclareApplet<Binary, 41, CAT_LOGIC | CAT_MODULATOR>
#endif
#endif
#ifndef DISABLE_APPLET_BitBeat
    , DeclareApplet<BitBeat, 79, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_BootsNCat
    , DeclareApplet<BootsNCat, 55, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_Brancher
    , DeclareApplet<Brancher, 4, CAT_UTILITY | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_BugCrack
    , DeclareApplet<BugCrack, 51, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_Burst
    , DeclareApplet<Burst, 31, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Button
    , DeclareApplet<Button, 65, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_Calculate
    , DeclareApplet<Calculate, 12, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_Calibr8
    , DeclareApplet<Calibr8, 88, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_Carpeggio
    , DeclareApplet<Carpeggio, 32, CAT_SEQUENCER | CAT_QUANTIZER>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_Chordinator
    , DeclareApplet<Chordinator, 64, CAT_QUANTIZER>
#endif
#endif
#ifndef DISABLE_APPLET_ClockDivider
    , DeclareApplet<ClockDivider, 6, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_ClkToGate
    , DeclareApplet<ClkToGate, 78, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_ClockSkip
    , DeclareApplet<ClockSkip, 28, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Combin8
    , DeclareApplet<Combin8, 82, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_Compare
    , DeclareApplet<Compare, 30, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_Cumulus
    , DeclareApplet<Cumulus, 5, CAT_LOGIC>
#endif
#ifndef DISABLE_APPLET_CVRecV2
    , DeclareApplet<CVRecV2, 24, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_CVSeq
    , DeclareApplet<CVSeq, 92, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_DivSeq
    , DeclareApplet<DivSeq, 68, CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_DivSeq10
    , DeclareApplet<DivSeq10, 80, CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_DrLoFi
    , DeclareApplet<DrLoFi, 16, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_DrumMap
    , DeclareApplet<DrumMap, 57, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_DualQuant
    , DeclareApplet<DualQuant, 9, CAT_QUANTIZER>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_OffsetQuant
    , DeclareApplet<OffsetQuant, 90, CAT_QUANTIZER>
#endif
#endif
#if !defined(CUSTOM_BUILD) || defined(PEWPEWPEW)
#ifndef DISABLE_APPLET_DuoTET
    , DeclareApplet<DuoTET, 63, CAT_QUANTIZER>
#endif
#endif
#ifndef DISABLE_APPLET_EbbAndLfo
    , DeclareApplet<EbbAndLfo, 7, CAT_MODULATOR>
#endif
#ifdef ENABLE_APP_ENIGMA
#ifndef DISABLE_APPLET_EnigmaJr
    , DeclareApplet<EnigmaJr, 45, CAT_SEQUENCER>
#endif
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_EnsOscKey
    , DeclareApplet<EnsOscKey, 35, CAT_QUANTIZER>
#endif
#endif
#ifndef DISABLE_APPLET_EnvFollow
    , DeclareApplet<EnvFollow, 42, CAT_UTILITY | CAT_MODULATOR>
#endif
#ifdef __IMXRT1062__
#ifndef DISABLE_APPLET_EnvSeq
    , DeclareApplet<EnvSeq, 91, CAT_SEQUENCER>
#endif
#endif
#ifndef DISABLE_APPLET_EuclidO
    , DeclareApplet<EuclidO, 83, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_EuclidX
    , DeclareApplet<EuclidX, 15, CAT_SEQUENCER>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_GameOfLife
    , DeclareApplet<GameOfLife, 22, CAT_MODULATOR>
#endif
#endif
#ifndef DISABLE_APPLET_GateDelay
    , DeclareApplet<GateDelay, 29, CAT_CLOCKING>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_GatedVCA
    , DeclareApplet<GatedVCA, 17, CAT_UTILITY | CAT_LOGIC>
#endif
#endif
#ifndef DISABLE_APPLET_Logic
    , DeclareApplet<Logic, 10, CAT_LOGIC | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_LowerRenz
    , DeclareApplet<LowerRenz, 21, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_Metronome
    , DeclareApplet<Metronome, 50, CAT_CLOCKING>
#endif
#ifdef __IMXRT1062__
#ifndef DISABLE_APPLET_MidiLoop
    , DeclareApplet<MidiLoop, 81, CAT_MIDI>
#endif
#endif
#ifndef DISABLE_APPLET_MarkoV
    , DeclareApplet<MarkoV, 93, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_MarkovPerc
    , DeclareApplet<MarkovPerc, 94, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_hMIDIIn
    , DeclareApplet<hMIDIIn, 150, CAT_MIDI>
#endif
#ifndef DISABLE_APPLET_hMIDIOut
    , DeclareApplet<hMIDIOut, 27, CAT_MIDI>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_MultiScale
    , DeclareApplet<MultiScale, 73, CAT_QUANTIZER>
#endif
#endif
#ifndef DISABLE_APPLET_Palimpsest
    , DeclareApplet<Palimpsest, 20, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_Pigeons
    , DeclareApplet<Pigeons, 71, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_PolyDiv
    , DeclareApplet<PolyDiv, 72, CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Ponglet
    , DeclareApplet<Ponglet, 99, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_ProbabilityDivider
    , DeclareApplet<ProbabilityDivider, 59, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_ProbabilityMelody
    , DeclareApplet<ProbabilityMelody, 62, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Relabi
    , DeclareApplet<Relabi, 89, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_ResetClock
    , DeclareApplet<ResetClock, 70, CAT_UTILITY | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_RndWalk
    , DeclareApplet<RndWalk, 69, CAT_MODULATOR>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_RunglBook
    , DeclareApplet<RunglBook, 44, CAT_MODULATOR>
#endif
#endif
#ifndef DISABLE_APPLET_ScaleDuet
    , DeclareApplet<ScaleDuet, 26, CAT_QUANTIZER>
#endif
#ifndef DISABLE_APPLET_Schmitt
    , DeclareApplet<Schmitt, 40, CAT_LOGIC>
#endif
#ifndef DISABLE_APPLET_Scope
    , DeclareApplet<Scope, 23, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_Seq32
    , DeclareApplet<Seq32, 75, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_SeqPlay7
    , DeclareApplet<SeqPlay7, 76, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_SequenceX
    , DeclareApplet<SequenceX, 14, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_ShiftGate
    , DeclareApplet<ShiftGate, 48, CAT_LOGIC | CAT_MODULATOR | CAT_CLOCKING>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_ShiftReg
    , DeclareApplet<ShiftReg, 77, CAT_LOGIC | CAT_MODULATOR | CAT_CLOCKING>
#endif
#endif
#ifndef DISABLE_APPLET_Shredder
    , DeclareApplet<Shredder, 58, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_Shuffle
    , DeclareApplet<Shuffle, 36, CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Slew
    , DeclareApplet<Slew, 19, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_Squanch
    , DeclareApplet<Squanch, 46, CAT_QUANTIZER>
#endif
#ifndef DISABLE_APPLET_Stairs
    , DeclareApplet<Stairs, 61, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_Strum
    , DeclareApplet<Strum, 74, CAT_QUANTIZER>
#endif
#ifndef DISABLE_APPLET_Switch
    , DeclareApplet<Switch, 3, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_SwitchSeq
    , DeclareApplet<SwitchSeq, 38, CAT_UTILITY>
#endif
#ifndef DISABLE_APPLET_TB_3PO
    , DeclareApplet<TB_3PO, 60, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_TLNeuron
    , DeclareApplet<TLNeuron, 13, CAT_LOGIC>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_Trending
    , DeclareApplet<Trending, 37, CAT_LOGIC>
#endif
#endif
#ifndef DISABLE_APPLET_TrigSeq
    , DeclareApplet<TrigSeq, 11, CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_TrigSeq16
    , DeclareApplet<TrigSeq16, 25, CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_TruthCat3
    , DeclareApplet<TruthCat3, 85, CAT_LOGIC | CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_TruthCat4
    , DeclareApplet<TruthCat4, 84, CAT_LOGIC | CAT_SEQUENCER | CAT_CLOCKING>
#endif
#ifndef DISABLE_APPLET_Tuner
    , DeclareApplet<Tuner, 39, CAT_OTHER>
#endif
#ifndef DISABLE_APPLET_TwoRings
    , DeclareApplet<TwoRings, 18, CAT_SEQUENCER>
#endif
#ifndef DISABLE_APPLET_VectorEG
    , DeclareApplet<VectorEG, 52, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_VectorLFO
    , DeclareApplet<VectorLFO, 49, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_VectorMod
    , DeclareApplet<VectorMod, 53, CAT_MODULATOR> // awkward middle child
#endif
#ifndef DISABLE_APPLET_VectorMorph
    , DeclareApplet<VectorMorph, 54, CAT_MODULATOR>
#endif
#ifndef DISABLE_APPLET_Voltage
    , DeclareApplet<Voltage, 43, CAT_UTILITY>
#endif
#ifdef PEWPEWPEW
#ifndef DISABLE_APPLET_WTVCO
    , DeclareApplet<WTVCO, 67, CAT_OTHER>
#endif
#endif
#ifndef DISABLE_APPLET_Xfader
    , DeclareApplet<Xfader, 33, CAT_UTILITY>
#endif
>{};


namespace HS {
  static constexpr auto appletIds = reg.getIds();
  constexpr int HEMISPHERE_AVAILABLE_APPLETS = appletIds.size();

  uint64_t hidden_applets[2] = { 0, 0 };
  bool applet_is_hidden(const int& index) {
    return (hidden_applets[index/64] >> (index%64)) & 1;
  }
  void showhide_applet(const int& index) {
    const int seg = index/64;
    hidden_applets[seg] = hidden_applets[seg] ^ (uint64_t(1) << (index%64));
  }

  HemisphereApplet * get_applet(const int index, HEM_SIDE slot = LEFT_HEMISPHERE) {
    return reg.get(appletIds[index], slot);
  }

  const char * get_applet_name(const int index) {
    return reg.getName(index);
  }

  const uint8_t * get_applet_icon(const int index) {
    return reg.getIcon(index);
  }

  constexpr int get_applet_index_by_id(const RegID id) {
    int index = 0;
    for (int i = 0; i < HEMISPHERE_AVAILABLE_APPLETS; i++)
    {
        if (appletIds[i] == id) index = i;
    }
    return index;
  }

  int get_next_applet_index(int index, const int dir) {
    do {
      index += dir;
      if (index >= HEMISPHERE_AVAILABLE_APPLETS) index = 0;
      if (index < 0) index = HEMISPHERE_AVAILABLE_APPLETS - 1;
    } while (applet_is_hidden(index));

    return index;
  }
}
