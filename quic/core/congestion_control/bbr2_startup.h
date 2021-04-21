// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_STARTUP_H_
#define QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_STARTUP_H_

#include "quic/core/congestion_control/bbr2_misc.h"
#include "quic/core/quic_bandwidth.h"
#include "quic/core/quic_time.h"
#include "quic/core/quic_types.h"
#include "quic/platform/api/quic_export.h"

namespace quic {

// 启动阶段引擎策略
class Bbr2Sender;
class QUIC_EXPORT_PRIVATE Bbr2StartupMode final : public Bbr2ModeBase {
 public:
  Bbr2StartupMode(const Bbr2Sender* sender,
                  Bbr2NetworkModel* model,
                  QuicTime now);

  void Enter(QuicTime now,
             const Bbr2CongestionEvent* congestion_event) override;
  void Leave(QuicTime now,
             const Bbr2CongestionEvent* congestion_event) override;

  Bbr2Mode OnCongestionEvent(
      QuicByteCount prior_in_flight,
      QuicTime event_time,
      const AckedPacketVector& acked_packets,
      const LostPacketVector& lost_packets,
      const Bbr2CongestionEvent& congestion_event) override;

  Limits<QuicByteCount> GetCwndLimits() const override {
    // Inflight_lo is never set in STARTUP.
    QUICHE_DCHECK_EQ(Bbr2NetworkModel::inflight_lo_default(),
                     model_->inflight_lo());
    return NoGreaterThan(model_->inflight_lo());
  }

  bool IsProbingForBandwidth() const override { return true; }

  Bbr2Mode OnExitQuiescence(QuicTime /*now*/,
                            QuicTime /*quiescence_start_time*/) override {
    return Bbr2Mode::STARTUP;
  }

  struct QUIC_EXPORT_PRIVATE DebugState {
    bool full_bandwidth_reached;
    QuicBandwidth full_bandwidth_baseline = QuicBandwidth::Zero();
    QuicRoundTripCount round_trips_without_bandwidth_growth;
  };

  DebugState ExportDebugState() const;

 private:
  const Bbr2Params& Params() const;
  // 检测是否发送了太多数据，bdp有没有增加
  void CheckExcessiveLosses(const Bbr2CongestionEvent& congestion_event);
  // Used when the pacing gain can decrease in STARTUP.
  QuicBandwidth max_bw_at_round_beginning_ = QuicBandwidth::Zero();
};

QUIC_EXPORT_PRIVATE std::ostream& operator<<(
    std::ostream& os,
    const Bbr2StartupMode::DebugState& state);

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_CONGESTION_CONTROL_BBR2_STARTUP_H_