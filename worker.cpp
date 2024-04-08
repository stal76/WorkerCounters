#include "worker.h"
#include "type.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

Worker::Worker(tCoreId core_id) : core_id_(core_id) {
}

uint64_t Worker::FillMetadataWorkerCounters(common::pde::MetadataWorker* metadata) {
	metadata->start_counters = 0;
	metadata->start_acl_counters = common::pde::AllignToSizeCacheLine(metadata->start_counters + YANET_CONFIG_COUNTERS_SIZE * sizeof(uint64_t));
	metadata->start_bursts = common::pde::AllignToSizeCacheLine(metadata->start_acl_counters + YANET_CONFIG_ACL_COUNTERS_SIZE * sizeof(uint64_t));
    metadata->start_stats = common::pde::AllignToSizeCacheLine(metadata->start_bursts + (CONFIG_YADECAP_MBUFS_BURST_SIZE + 1) * sizeof(uint64_t));
    metadata->start_stats_ports = common::pde::AllignToSizeCacheLine(metadata->start_stats + sizeof(common::worker::stats::common));
	uint64_t start_next = common::pde::AllignToSizeCacheLine(metadata->start_stats_ports + CONFIG_YADECAP_PORTS_SIZE * sizeof(common::worker::stats::port)); // ?
	metadata->total_size = start_next - 0;
	metadata->UpdateIndexes();

	// stats
	static_assert(std::is_trivially_destructible<common::worker::stats::common>::value, "invalid struct destructible");
	std::map<std::string, uint64_t> counters_stats;
	counters_stats["brokenPackets"] = offsetof(common::worker::stats::common, brokenPackets);
	counters_stats["dropPackets"] = offsetof(common::worker::stats::common, dropPackets);
	counters_stats["ring_highPriority_drops"] = offsetof(common::worker::stats::common, ring_highPriority_drops);
	counters_stats["ring_normalPriority_drops"] = offsetof(common::worker::stats::common, ring_normalPriority_drops);
	counters_stats["ring_lowPriority_drops"] = offsetof(common::worker::stats::common, ring_lowPriority_drops);
	counters_stats["ring_highPriority_packets"] = offsetof(common::worker::stats::common, ring_highPriority_packets);
	counters_stats["ring_normalPriority_packets"] = offsetof(common::worker::stats::common, ring_normalPriority_packets);
	counters_stats["ring_lowPriority_packets"] = offsetof(common::worker::stats::common, ring_lowPriority_packets);
	counters_stats["decap_packets"] = offsetof(common::worker::stats::common, decap_packets);
	counters_stats["decap_fragments"] = offsetof(common::worker::stats::common, decap_fragments);
	counters_stats["decap_unknownExtensions"] = offsetof(common::worker::stats::common, decap_unknownExtensions);
	counters_stats["interface_lookupMisses"] = offsetof(common::worker::stats::common, interface_lookupMisses);
	counters_stats["interface_hopLimits"] = offsetof(common::worker::stats::common, interface_hopLimits);
	counters_stats["interface_neighbor_invalid"] = offsetof(common::worker::stats::common, interface_neighbor_invalid);
	counters_stats["nat64stateless_ingressPackets"] = offsetof(common::worker::stats::common, nat64stateless_ingressPackets);
	counters_stats["nat64stateless_ingressFragments"] = offsetof(common::worker::stats::common, nat64stateless_ingressFragments);
	counters_stats["nat64stateless_ingressUnknownICMP"] = offsetof(common::worker::stats::common, nat64stateless_ingressUnknownICMP);
	counters_stats["nat64stateless_egressPackets"] = offsetof(common::worker::stats::common, nat64stateless_egressPackets);
	counters_stats["nat64stateless_egressFragments"] = offsetof(common::worker::stats::common, nat64stateless_egressFragments);
	counters_stats["nat64stateless_egressUnknownICMP"] = offsetof(common::worker::stats::common, nat64stateless_egressUnknownICMP);
	counters_stats["balancer_invalid_reals_count"] = offsetof(common::worker::stats::common, balancer_invalid_reals_count);
	counters_stats["fwsync_multicast_egress_drops"] = offsetof(common::worker::stats::common, fwsync_multicast_egress_drops);
	counters_stats["fwsync_multicast_egress_packets"] = offsetof(common::worker::stats::common, fwsync_multicast_egress_packets);
	counters_stats["fwsync_multicast_egress_imm_packets"] = offsetof(common::worker::stats::common, fwsync_multicast_egress_imm_packets);
	counters_stats["fwsync_no_config_drops"] = offsetof(common::worker::stats::common, fwsync_no_config_drops);
	counters_stats["fwsync_unicast_egress_drops"] = offsetof(common::worker::stats::common, fwsync_unicast_egress_drops);
	counters_stats["fwsync_unicast_egress_packets"] = offsetof(common::worker::stats::common, fwsync_unicast_egress_packets);
	counters_stats["acl_ingress_dropPackets"] = offsetof(common::worker::stats::common, acl_ingress_dropPackets);
	counters_stats["acl_egress_dropPackets"] = offsetof(common::worker::stats::common, acl_egress_dropPackets);
	counters_stats["repeat_ttl"] = offsetof(common::worker::stats::common, repeat_ttl);
	counters_stats["leakedMbufs"] = offsetof(common::worker::stats::common, leakedMbufs);
	counters_stats["logs_packets"] = offsetof(common::worker::stats::common, logs_packets);
	counters_stats["logs_drops"] = offsetof(common::worker::stats::common, logs_drops);
	for (const auto& iter : counters_stats) {
	    metadata->counter_positions[iter.first] = (metadata->start_stats + iter.second) / sizeof(uint64_t);
    }

	// counters
	std::map<std::string, common::globalBase::static_counter_type> counters_named;
	counters_named["balancer_state_insert_failed"] = common::globalBase::static_counter_type::balancer_state_insert_failed;
	counters_named["balancer_state_insert_done"] = common::globalBase::static_counter_type::balancer_state_insert_done;
	counters_named["balancer_icmp_generated_echo_reply_ipv4"] = common::globalBase::static_counter_type::balancer_icmp_generated_echo_reply_ipv4;
	counters_named["balancer_icmp_generated_echo_reply_ipv6"] = common::globalBase::static_counter_type::balancer_icmp_generated_echo_reply_ipv6;
	counters_named["balancer_icmp_drop_icmpv4_payload_too_short_ip"] = common::globalBase::static_counter_type::balancer_icmp_drop_icmpv4_payload_too_short_ip;
	counters_named["balancer_icmp_drop_icmpv4_payload_too_short_port"] = common::globalBase::static_counter_type::balancer_icmp_drop_icmpv4_payload_too_short_port;
	counters_named["balancer_icmp_drop_icmpv6_payload_too_short_ip"] = common::globalBase::static_counter_type::balancer_icmp_drop_icmpv6_payload_too_short_ip;
	counters_named["balancer_icmp_drop_icmpv6_payload_too_short_port"] = common::globalBase::static_counter_type::balancer_icmp_drop_icmpv6_payload_too_short_port;
	counters_named["balancer_icmp_unmatching_src_from_original_ipv4"] = common::globalBase::static_counter_type::balancer_icmp_unmatching_src_from_original_ipv4;
	counters_named["balancer_icmp_unmatching_src_from_original_ipv6"] = common::globalBase::static_counter_type::balancer_icmp_unmatching_src_from_original_ipv6;
	counters_named["balancer_icmp_drop_real_disabled"] = common::globalBase::static_counter_type::balancer_icmp_drop_real_disabled;
	counters_named["balancer_icmp_no_balancer_src_ipv4"] = common::globalBase::static_counter_type::balancer_icmp_no_balancer_src_ipv4;
	counters_named["balancer_icmp_no_balancer_src_ipv6"] = common::globalBase::static_counter_type::balancer_icmp_no_balancer_src_ipv6;
	counters_named["balancer_icmp_drop_already_cloned"] = common::globalBase::static_counter_type::balancer_icmp_drop_already_cloned;
	counters_named["balancer_icmp_drop_no_unrdup_table_for_balancer_id"] = common::globalBase::static_counter_type::balancer_icmp_drop_no_unrdup_table_for_balancer_id;
	counters_named["balancer_icmp_drop_unrdup_vip_not_found"] = common::globalBase::static_counter_type::balancer_icmp_drop_unrdup_vip_not_found;
	counters_named["balancer_icmp_drop_no_vip_vport_proto_table_for_balancer_id"] = common::globalBase::static_counter_type::balancer_icmp_drop_no_vip_vport_proto_table_for_balancer_id;
	counters_named["balancer_icmp_drop_unexpected_transport_protocol"] = common::globalBase::static_counter_type::balancer_icmp_drop_unexpected_transport_protocol;
	counters_named["balancer_icmp_drop_unknown_service"] = common::globalBase::static_counter_type::balancer_icmp_drop_unknown_service;
	counters_named["balancer_icmp_failed_to_clone"] = common::globalBase::static_counter_type::balancer_icmp_failed_to_clone;
	counters_named["balancer_icmp_clone_forwarded"] = common::globalBase::static_counter_type::balancer_icmp_clone_forwarded;
	counters_named["balancer_icmp_sent_to_real"] = common::globalBase::static_counter_type::balancer_icmp_sent_to_real;
	counters_named["balancer_icmp_out_rate_limit_reached"] = common::globalBase::static_counter_type::balancer_icmp_out_rate_limit_reached;
	counters_named["slow_worker_normal_priority_rate_limit_exceeded"] = common::globalBase::static_counter_type::slow_worker_normal_priority_rate_limit_exceeded;

	counters_named["acl_ingress_v4_broken_packet"] = common::globalBase::static_counter_type::acl_ingress_v4_broken_packet;
	counters_named["acl_ingress_v6_broken_packet"] = common::globalBase::static_counter_type::acl_ingress_v6_broken_packet;
	counters_named["acl_egress_v4_broken_packet"] = common::globalBase::static_counter_type::acl_egress_v4_broken_packet;
	counters_named["acl_egress_v6_broken_packet"] = common::globalBase::static_counter_type::acl_egress_v6_broken_packet;
	// No balancer_fragment_drops
	counters_named["balancer_fragment_drops"] = common::globalBase::static_counter_type::balancer_fragment_drops;

	for (const auto& iter : counters_named) {
        metadata->counter_positions[iter.first] = metadata->start_counters / sizeof(uint64_t) + static_cast<uint64_t>(iter.second);
    }

	return start_next;
}

void Worker::SetBufferForCounters(void* buffer, const common::pde::MetadataWorker& metadata) {
	// std::cout << "Set buffer: " << buffer << "\n";
	counters = static_cast<uint64_t*>(common::pde::PtrAdd(buffer, metadata.start_counters));
	aclCounters = static_cast<uint64_t*>(common::pde::PtrAdd(buffer, metadata.start_acl_counters));
	bursts = static_cast<uint64_t*>(common::pde::PtrAdd(buffer, metadata.start_bursts));
	stats = static_cast<common::worker::stats::common*>(common::pde::PtrAdd(buffer, metadata.start_stats));
	statsPorts = static_cast<common::worker::stats::port*>(common::pde::PtrAdd(buffer, metadata.start_stats_ports));
}

void Worker::Start() {
	main_trhead = new std::thread([this]() {
		MainThread();
	});
	std::cout << "Start worker " << core_id_ << " thread\n";
}

void Worker::Join() {
	if (main_trhead != nullptr) {
		main_trhead->join();
		delete main_trhead;
	}
}

void Worker::MainThread() {
	uint64_t prefix_core = (((uint64_t)core_id_ << 8) + 0xfe) << 48;
	std::vector<uint64_t*> cnts = {&stats->decap_fragments, &aclCounters[17], &bursts[7], &statsPorts[CONFIG_YADECAP_PORTS_SIZE - 1].physicalPort_egress_drops,
								   &counters[(uint32_t)common::globalBase::static_counter_type::balancer_icmp_generated_echo_reply_ipv6]};
	for (uint64_t* ptr : cnts) {
		*ptr = prefix_core;
	}

	while (true) {
		for (uint64_t* ptr : cnts) {
			(*ptr)++;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
