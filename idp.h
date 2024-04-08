#pragma once

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
