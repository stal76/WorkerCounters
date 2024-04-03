#pragma once

#include <map>
#include <set>
#include <stdint.h>
#include <string>

#include "common.h"
#include "type.h"

namespace common
{

namespace processes_data_exchange
{


class BufferWriter {
public:
    BufferWriter(void* buffer, uint64_t total_size);
    
    static uint64_t AllignToSizeCacheLine(uint64_t size);
    static uint64_t Size(uint64_t size);
    static uint64_t SizeOfStringIn64(const std::string& str);

    void StartBlock(const char* block_name, uint64_t size);
    void Write64(uint64_t value);
    void WriteString(const std::string& str);

private:
    void* buffer_;
    uint64_t total_size_;

    void* buffer_next_;
    void* buffer_current_;
    uint64_t current_size_ = 0;
    const char* block_name_;
};

class BufferReader {
public:
    BufferReader(void* buffer, uint64_t total_size);
    void StartBlock(const char* block_name);
    uint64_t Read64();
    std::string ReadString();

private:
    void* buffer_;
    uint64_t total_size_;

    void* buffer_next_;
    void* buffer_current_;
    uint64_t current_size_ = 0;
    const char* block_name_;
};

void* PtrAdd(void* buf, uint64_t count);

struct SocketsInfo {
    std::map<tSocketId, void*> all_sockets;
    
    uint64_t Size();
    void WriteToBuffer(BufferWriter& writer);
    void ReadFromBuffer(BufferReader& reader);
};

struct WorkersInfo {
    struct OneWorkerInfo {
        tSocketId socket_id;
        uint64_t start_at_memory;
    };
    std::map<tCoreId, OneWorkerInfo> workers;

    uint64_t Size();
    void WriteToBuffer(BufferWriter& writer);
    void ReadFromBuffer(BufferReader& reader);
};

struct MetadataWorkerCounters {
    uint64_t start_counters;
    uint64_t start_acl_counters;
    uint64_t start_bursts;
    uint64_t start_stats;
    uint64_t start_stats_ports;

    uint64_t total_size;    // size of all data from this structure

    std::map<std::string, uint64_t> counter_positions;
    void AddCounters(const std::map<std::string, void*>& counters_stats, uint64_t start, void* base_addr);
    void AddCounters(const std::map<std::string, common::globalBase::static_counter_type>& counters_named, uint64_t start);

    uint64_t Size();
    void WriteToBuffer(BufferWriter& writer);
    void ReadFromBuffer(BufferReader& reader);

    uint64_t index_counters;
    uint64_t index_acl_counters;
    uint64_t index_bursts;
    uint64_t index_stats_ports;
    void UpdateIndexes();
};

struct MetadataWorkerGcCounters {
    uint64_t start_counters;
    uint64_t start_stats;

    uint64_t total_size;

    std::map<std::string, uint64_t> counter_positions;
    void AddCounters(const std::map<std::string, void*>& counters_stats, uint64_t start, void* base_addr);

    uint64_t Size();
    void WriteToBuffer(BufferWriter& writer);
    void ReadFromBuffer(BufferReader& reader);

    uint64_t index_counters;
};

struct MetadataOtherCounters {
    uint64_t start_bus_errors;
    uint64_t start_bus_requests;

    uint64_t Size();
    uint64_t Initialize(uint64_t start);    
    void WriteToBuffer(BufferWriter& writer);
    void ReadFromBuffer(BufferReader& reader);
};

struct MainFileData {
    SocketsInfo sockets;
    WorkersInfo workers;
    WorkersInfo workers_gc;
    MetadataWorkerCounters metadata_workers;
    MetadataWorkerGcCounters metadata_workers_gc;
    MetadataOtherCounters metadata_other_counters;

    void AddWorker(tSocketId socket_id, tCoreId core_id);
    void AddWorkerGC(tSocketId socket_id, tCoreId core_id);
    eResult FinalizeDataplane(bool useHugeMem);
    std::string GetNameOfSocketFileName(tSocketId socket_id);

    eResult ReadFromDataplane(bool useHugeMem, bool open_sockets_file);

    void* GetStartBufferForWorker(tCoreId core_id);
    void* GetStartBufferForWorkerGC(tCoreId core_id);

    void* main_buffer = nullptr;
    std::map<tSocketId, void*> buffers_by_sockets;
    uint64_t* GetStartBufferOtherCounters(uint64_t start);
};

}  // namespace processes_data_exchange

}  // namespace common
