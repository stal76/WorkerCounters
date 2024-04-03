#pragma once

#include <stdint.h>

using tSocketId = uint32_t;
using tCoreId = uint32_t;

#define YANET_CONFIG_COUNTERS_SIZE 100
#define YANET_CONFIG_ACL_COUNTERS_SIZE 100
#define CONFIG_YADECAP_MBUFS_BURST_SIZE 32
#define CONFIG_YADECAP_PORTS_SIZE 32
#define YANET_CONFIG_COUNTER_FALLBACK_SIZE 32

namespace common
{
namespace worker
{
namespace stats
{
struct common
{
	uint64_t brokenPackets;
	uint64_t dropPackets;
	uint64_t ring_highPriority_drops;
	uint64_t ring_normalPriority_drops;
	uint64_t ring_lowPriority_drops;
	uint64_t ring_highPriority_packets;
	uint64_t ring_normalPriority_packets;
	uint64_t ring_lowPriority_packets;

	/// @todo: use counters
	uint64_t decap_packets;
	uint64_t decap_fragments;
	uint64_t decap_unknownExtensions;
	uint64_t interface_lookupMisses;
	uint64_t interface_hopLimits;
	uint64_t interface_neighbor_invalid;
	uint64_t nat64stateless_ingressPackets;
	uint64_t nat64stateless_ingressFragments;
	uint64_t nat64stateless_ingressUnknownICMP;
	uint64_t nat64stateless_egressPackets;
	uint64_t nat64stateless_egressFragments;
	uint64_t nat64stateless_egressUnknownICMP;
	uint64_t balancer_invalid_reals_count;
	uint64_t fwsync_multicast_egress_drops;
	uint64_t fwsync_multicast_egress_packets;
	uint64_t fwsync_multicast_egress_imm_packets;
	uint64_t fwsync_no_config_drops;
	uint64_t fwsync_unicast_egress_drops;
	uint64_t fwsync_unicast_egress_packets;
	uint64_t acl_ingress_dropPackets;
	uint64_t acl_egress_dropPackets;
	uint64_t repeat_ttl;
	uint64_t leakedMbufs;
	uint64_t logs_packets;
	uint64_t logs_drops;
};

struct port
{
	/*
    port()
	{
		memset(this, 0, sizeof(*this));
	}

	void pop(stream_in_t& stream)
	{
		stream.pop((char*)this, sizeof(*this));
	}

	void push(stream_out_t& stream) const
	{
		stream.push((char*)this, sizeof(*this));
	}
    */

	uint64_t physicalPort_egress_drops;
	uint64_t controlPlane_drops; ///< @todo: DELETE
};
}
}

namespace worker_gc
{
struct stats_t
{
	/// @todo
	uint64_t broken_packets;
	uint64_t drop_packets;
	uint64_t ring_to_slowworker_packets;
	uint64_t ring_to_slowworker_drops;
	uint64_t fwsync_multicast_egress_packets;
	uint64_t fwsync_multicast_egress_drops;
	uint64_t fwsync_unicast_egress_packets;
	uint64_t fwsync_unicast_egress_drops;
	uint64_t drop_samples;
	uint64_t balancer_state_insert_failed;
	uint64_t balancer_state_insert_done;
};
}

namespace globalBase ///< @todo: remove
{
enum class static_counter_type : uint32_t
{
	start = YANET_CONFIG_COUNTER_FALLBACK_SIZE - 1,
	balancer_state,
	balancer_state_insert_failed = balancer_state,
	balancer_state_insert_done,
	balancer_icmp_generated_echo_reply_ipv4,
	balancer_icmp_generated_echo_reply_ipv6,
	balancer_icmp_sent_to_real,
	balancer_icmp_drop_icmpv4_payload_too_short_ip,
	balancer_icmp_drop_icmpv4_payload_too_short_port,
	balancer_icmp_drop_icmpv6_payload_too_short_ip,
	balancer_icmp_drop_icmpv6_payload_too_short_port,
	balancer_icmp_unmatching_src_from_original_ipv4,
	balancer_icmp_unmatching_src_from_original_ipv6,
	balancer_icmp_drop_real_disabled,
	balancer_icmp_no_balancer_src_ipv4,
	balancer_icmp_no_balancer_src_ipv6,
	balancer_icmp_drop_already_cloned,
	balancer_icmp_out_rate_limit_reached,
	balancer_icmp_drop_no_unrdup_table_for_balancer_id,
	balancer_icmp_drop_unrdup_vip_not_found,
	balancer_icmp_drop_no_vip_vport_proto_table_for_balancer_id,
	balancer_icmp_drop_unexpected_transport_protocol,
	balancer_icmp_drop_unknown_service,
	balancer_icmp_failed_to_clone,
	balancer_icmp_clone_forwarded,
	balancer_fragment_drops,
	acl_ingress_v4_broken_packet,
	acl_ingress_v6_broken_packet,
	acl_egress_v4_broken_packet,
	acl_egress_v6_broken_packet,
	slow_worker_normal_priority_rate_limit_exceeded,
	size
};
}

}

namespace common::idp
{
enum class errorType : uint32_t
{
	busRead,
	busWrite,
	busParse,
	size
};

enum class requestType : uint32_t
{
	updateGlobalBase,
	updateGlobalBaseBalancer,
	getGlobalBase,
	getWorkerStats,
	getSlowWorkerStats,
	get_worker_gc_stats,
	get_dregress_counters,
	get_ports_stats,
	get_ports_stats_extended,
	getControlPlanePortStats,
	getPortStatsEx,
	getCounters,
	getFragmentationStats,
	getFWState,
	getFWStateStats,
	clearFWState,
	getAclCounters,
	getOtherStats,
	getConfig,
	getErrors,
	getReport,
	lpm4LookupAddress,
	lpm6LookupAddress,
	nat64stateful_state,
	balancer_connection,
	balancer_service_connections,
	balancer_real_connections,
	limits,
	samples,
	debug_latch_update,
	unrdup_vip_to_balancers,
	update_vip_vport_proto,
	version,
	get_counter_by_name,
	get_shm_info,
	get_shm_tsc_info,
	set_shm_tsc_state,
	dump_physical_port,
	balancer_state_clear,
	neighbor_show,
	neighbor_insert,
	neighbor_remove,
	neighbor_clear,
	neighbor_flush,
	neighbor_update_interfaces,
	neighbor_stats,
	memory_manager_update,
	memory_manager_stats,
	size, // size should always be at the bottom of the list, this enum allows us to find out the size of the enum list
};

}  // namespace common::idp
