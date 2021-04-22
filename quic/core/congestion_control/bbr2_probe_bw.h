// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_PROBE_BW_H_
#define QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_PROBE_BW_H_

#include <cstdint>

#include "quic/core/congestion_control/bbr2_misc.h"
#include "quic/core/quic_time.h"
#include "quic/core/quic_types.h"
#include "quic/platform/api/quic_export.h"
#include "quic/platform/api/quic_flags.h"

namespace quic {

// 稳定阶段策略引擎
class Bbr2Sender;
class QUIC_EXPORT_PRIVATE Bbr2ProbeBwMode final : public Bbr2ModeBase {
 public:
  using Bbr2ModeBase::Bbr2ModeBase;

  void Enter(QuicTime now,
             const Bbr2CongestionEvent* congestion_event) override;
  void Leave(QuicTime /*now*/,
             const Bbr2CongestionEvent* /*congestion_event*/) override {}

  Bbr2Mode OnCongestionEvent(
      QuicByteCount prior_in_flight,
      QuicTime event_time,
      const AckedPacketVector& acked_packets,
      const LostPacketVector& lost_packets,
      const Bbr2CongestionEvent& congestion_event) override;

  Limits<QuicByteCount> GetCwndLimits() const override;

  bool IsProbingForBandwidth() const override;

  Bbr2Mode OnExitQuiescence(QuicTime now,
                            QuicTime quiescence_start_time) override;

  enum class CyclePhase : uint8_t {
    PROBE_NOT_STARTED,
    PROBE_UP,
    PROBE_DOWN,
    PROBE_CRUISE,
    PROBE_REFILL,
  };

  static const char* CyclePhaseToString(CyclePhase phase);

  struct QUIC_EXPORT_PRIVATE DebugState {
    CyclePhase phase;
    QuicTime cycle_start_time = QuicTime::Zero();
    QuicTime phase_start_time = QuicTime::Zero();
  };

  DebugState ExportDebugState() const;

 private:
  const Bbr2Params& Params() const;
  float PacingGainForPhase(CyclePhase phase) const;

  void UpdateProbeUp(QuicByteCount prior_in_flight,
                     const Bbr2CongestionEvent& congestion_event);
  void UpdateProbeDown(QuicByteCount prior_in_flight,
                       const Bbr2CongestionEvent& congestion_event);
  void UpdateProbeCruise(const Bbr2CongestionEvent& congestion_event);
  void UpdateProbeRefill(const Bbr2CongestionEvent& congestion_event);

  enum AdaptUpperBoundsResult : uint8_t {
    ADAPTED_OK,
    ADAPTED_PROBED_TOO_HIGH,
    NOT_ADAPTED_INFLIGHT_HIGH_NOT_SET,
    NOT_ADAPTED_INVALID_SAMPLE,
  };

  // 调整带宽值。设置medol的inflight
  // Return whether adapted inflight_hi. If inflight is too high, this function
  // will not adapt inflight_hi and will return false.
  AdaptUpperBoundsResult MaybeAdaptUpperBounds(
      const Bbr2CongestionEvent& congestion_event);

  // 开启新一轮探测周期，主要初始化Cycle成员
  // 进入带宽降低阶段
  void EnterProbeDown(bool probed_too_high,
                      bool stopped_risky_probe,
                      QuicTime now);
  // 进入巡航阶段
  void EnterProbeCruise(QuicTime now);
  // 新一轮探测
  void EnterProbeRefill(uint64_t probe_up_rounds, QuicTime now);
  // 进入带宽提升阶段
  void EnterProbeUp(QuicTime now);

  // Call right before the exit of PROBE_DOWN.
  void ExitProbeDown();

  float PercentTimeElapsedToProbeBandwidth(
      const Bbr2CongestionEvent& congestion_event) const;

  // 新一轮探测带宽开始？
  bool IsTimeToProbeBandwidth(
      const Bbr2CongestionEvent& congestion_event) const;
  // 在ProbeDown阶段已经超过一个最小rtt时间
  bool HasStayedLongEnoughInProbeDown(
      const Bbr2CongestionEvent& congestion_event) const;
  // 到达本轮探测结束时间
  bool HasCycleLasted(QuicTime::Delta duration,
                      const Bbr2CongestionEvent& congestion_event) const;
  bool HasPhaseLasted(QuicTime::Delta duration,
                      const Bbr2CongestionEvent& congestion_event) const;
  bool IsTimeToProbeForRenoCoexistence(
      double probe_wait_fraction,
      const Bbr2CongestionEvent& congestion_event) const;

  void RaiseInflightHighSlope();
  // 可能提高inflight_hi
  void ProbeInflightHighUpward(const Bbr2CongestionEvent& congestion_event);

  struct QUIC_EXPORT_PRIVATE Cycle {
    QuicTime cycle_start_time = QuicTime::Zero();
    CyclePhase phase = CyclePhase::PROBE_NOT_STARTED;
    uint64_t rounds_in_phase = 0;
    QuicTime phase_start_time = QuicTime::Zero();
    QuicRoundTripCount rounds_since_probe = 0;
    QuicTime::Delta probe_wait_time = QuicTime::Delta::Zero();
    uint64_t probe_up_rounds = 0;
    QuicByteCount probe_up_bytes = std::numeric_limits<QuicByteCount>::max();
    QuicByteCount probe_up_acked = 0;
    // Whether max bandwidth filter window has advanced in this cycle. It is
    // advanced once per cycle.
    bool has_advanced_max_bw = false;
    bool is_sample_from_probing = false;
  } cycle_;

  // 上一轮探测过高
  bool last_cycle_probed_too_high_;
  bool last_cycle_stopped_risky_probe_;
};

QUIC_EXPORT_PRIVATE std::ostream& operator<<(
    std::ostream& os,
    const Bbr2ProbeBwMode::DebugState& state);

QUIC_EXPORT_PRIVATE std::ostream& operator<<(
    std::ostream& os,
    const Bbr2ProbeBwMode::CyclePhase phase);

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_PROBE_BW_H_
