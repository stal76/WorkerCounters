#include "processes_data_exchange.h"
#include "shared_memory.h"

#include <iostream>

#define CACHE_LINE_SIZE 64
#define SHARED_FILENAME_MAIN "/yanet_main.shm"
#define SHARED_FILENAME_SOCKET_PREFIX "/yanet_socket_"

namespace common
{

namespace processes_data_exchange
{

uint64_t PtrDiff64(void* first, void* second) {
    return (uint64_t*)first - (uint64_t*)second;
}

void* PtrAdd(void* buf, uint64_t count) {
    return (char*)buf + count;
}

// ----------------------------------------------------------------------------

BufferWriter::BufferWriter(void* buffer, uint64_t total_size) : buffer_(buffer), total_size_(total_size), buffer_next_(buffer) {
}

uint64_t BufferWriter::AllignToSizeCacheLine(uint64_t size) {
    return (size + (CACHE_LINE_SIZE - 1)) / CACHE_LINE_SIZE * CACHE_LINE_SIZE;
}

uint64_t BufferWriter::Size(uint64_t size) {
    return AllignToSizeCacheLine((1 + size) * sizeof(uint64_t));
}

uint64_t BufferWriter::SizeOfStringIn64(const std::string& str) {
    return str.size() / 8 + 1;
}

void BufferWriter::StartBlock(const char* block_name, uint64_t size) {
    block_name_ = block_name;
    buffer_current_ = buffer_next_;
    current_size_ = size;
    buffer_next_ = PtrAdd(buffer_next_, current_size_);
    Write64(size);
}

void BufferWriter::Write64(uint64_t value) {
    *(uint64_t*)(buffer_current_) = value;
    buffer_current_ = PtrAdd(buffer_current_, 8);
}

void BufferWriter::WriteString(const std::string& str) {
    uint64_t size = SizeOfStringIn64(str) * 8;
    char* buf = (char*)buffer_current_;
    *buf = str.size();
    for (size_t index = 0; index < str.size(); ++index) {
        buf[index + 1] = str[index];
    }
    buffer_current_ = buf + size;
}

BufferReader::BufferReader(void* buffer, uint64_t total_size) : buffer_(buffer), total_size_(total_size), buffer_next_(buffer) {
}

void BufferReader::StartBlock(const char* block_name) {
    block_name_ = block_name;
    buffer_current_ = buffer_next_;
    current_size_ = Read64();
    buffer_next_ = PtrAdd(buffer_next_, current_size_);
}

uint64_t BufferReader::Read64() {
    uint64_t result = *(uint64_t*)buffer_current_;
    buffer_current_ = PtrAdd(buffer_current_, 8);
    return result;
}

std::string BufferReader::ReadString() {
    char* buffer = (char*)buffer_current_;
    uint64_t size = (uint8_t)*buffer;
    std::string result;
    result.reserve(size);
    for (uint64_t index = 0; index < size; ++index) {
        result.push_back(buffer[index + 1]);
    }
    buffer_current_ = buffer + BufferWriter::SizeOfStringIn64(result) * sizeof(uint64_t);
    return result;
}

// ----------------------------------------------------------------------------

uint64_t SocketsInfo::Size() {
    // First value - numbers of sockets
    return BufferWriter::Size(1 + all_sockets.size());
}

void SocketsInfo::WriteToBuffer(BufferWriter& writer) {
    writer.StartBlock("Sockets", Size());
    writer.Write64(all_sockets.size());
    for (auto [socket_id, _] : all_sockets) {
        writer.Write64(socket_id);
    }
}

void SocketsInfo::ReadFromBuffer(BufferReader& reader) {
    reader.StartBlock("Sockets");
    uint64_t count = reader.Read64();
    for (uint64_t index = 0; index < count; ++index) {
        uint64_t socket_id = reader.Read64();
        // std::cout << "Socket: " << socket_id << "\n";
        all_sockets[(tSocketId)socket_id] = nullptr;
    }
}

uint64_t WorkersInfo::Size() {
    // First value - numbers of workers
    return BufferWriter::Size(1 + 3 * workers.size());
}

void WorkersInfo::WriteToBuffer(BufferWriter& writer) {
    writer.StartBlock("Workers", Size());
    writer.Write64(workers.size());
    for (const auto& iter : workers) {
        writer.Write64(iter.first);
        writer.Write64(iter.second.socket_id);
        writer.Write64(iter.second.start_at_memory);
    }
}

void WorkersInfo::ReadFromBuffer(BufferReader& reader) {
    reader.StartBlock("Sockets");
    uint64_t count = reader.Read64();
    for (uint64_t index = 0; index < count; ++index) {
        uint64_t core_id = reader.Read64();
        uint64_t socket_id = reader.Read64();
        uint64_t start_at_memory = reader.Read64();
        // std::cout << "Core: " << core_id << ", socket: " << socket_id << ", memory: " << start_at_memory << "\n";
        workers[(tCoreId)core_id] = {(tSocketId)socket_id, start_at_memory};
    }
}

// ----------------------------------------------------------------------------
// Metadata worker and worker_gc counters

void MetadataWorkerCounters::AddCounters(const std::map<std::string, void*>& counters_stats, uint64_t start, void* base_addr) {
    for (const auto& iter : counters_stats) {
	    counter_positions[iter.first] = start / 8 + PtrDiff64(iter.second, base_addr);
    }
}

void MetadataWorkerCounters::AddCounters(const std::map<std::string, common::globalBase::static_counter_type>& counters_named, uint64_t start) {
    for (const auto& iter : counters_named) {
        counter_positions[iter.first] = start / 8 + static_cast<uint64_t>(iter.second);
    }
}

uint64_t MetadataWorkerCounters::Size() {
    uint64_t size_of_names = 0;
    for (const auto& iter : counter_positions) {
        size_of_names += BufferWriter::SizeOfStringIn64(iter.first);
    }
    // 6 start positions + map (1 (size) + size (values) + size_of_names)
    return BufferWriter::Size(6 + 1 + counter_positions.size() + size_of_names);
}

void MetadataWorkerCounters::WriteToBuffer(BufferWriter& writer) {
    writer.StartBlock("MetadataWorkerCounters", Size());
    
    writer.Write64(start_counters);
    writer.Write64(start_acl_counters);
    writer.Write64(start_bursts);
    writer.Write64(start_stats);
    writer.Write64(start_stats_ports);
    writer.Write64(total_size);

    writer.Write64(counter_positions.size());
    for (const auto& iter : counter_positions) {
        writer.WriteString(iter.first);
        writer.Write64(iter.second);
    }
}

void MetadataWorkerCounters::ReadFromBuffer(BufferReader& reader) {
    reader.StartBlock("MetadataWorkerCounters");

    start_counters = reader.Read64();
    start_acl_counters = reader.Read64();
    start_bursts = reader.Read64();
    start_stats = reader.Read64();
    start_stats_ports = reader.Read64();
    total_size = reader.Read64();

    uint64_t size_positions = reader.Read64();
    for (uint64_t index = 0; index < size_positions; ++index) {
        std::string name = reader.ReadString();
        uint64_t value = reader.Read64();
        // std::cout << "\t" << name << " = " << value << "\n";
        counter_positions[name] = value;
    }

    UpdateIndexes();
}

void MetadataWorkerCounters::UpdateIndexes() {
    index_counters = start_counters / 8;
    index_acl_counters = start_acl_counters / 8;
    index_bursts = start_bursts / 8;
    index_stats_ports = start_stats_ports / 8;
}

void MetadataWorkerGcCounters::AddCounters(const std::map<std::string, void*>& counters_stats, uint64_t start, void* base_addr) {
    for (const auto& iter : counters_stats) {
	    counter_positions[iter.first] = start / 8 + PtrDiff64(iter.second, base_addr);
    }
}

uint64_t MetadataWorkerGcCounters::Size() {
    uint64_t size_of_names = 0;
    for (const auto& iter : counter_positions) {
        size_of_names += BufferWriter::SizeOfStringIn64(iter.first);
    }
    // 3 start positions + map (1 (size) + size (values) + size_of_names)
    return BufferWriter::Size(3 + 1 + counter_positions.size() + size_of_names);
}

void MetadataWorkerGcCounters::WriteToBuffer(BufferWriter& writer) {
    writer.StartBlock("MetadataWorkerGcCounters", Size());
    
    writer.Write64(start_counters);
    writer.Write64(start_stats);
    writer.Write64(total_size);

    // std::cout << "\tMetadataWorkerGcCounters: " << start_counters << " " << start_stats << " " << total_size << "\n";

    writer.Write64(counter_positions.size());
    for (const auto& iter : counter_positions) {
        writer.WriteString(iter.first);
        writer.Write64(iter.second);
    }
}

void MetadataWorkerGcCounters::ReadFromBuffer(BufferReader& reader) {
    reader.StartBlock("MetadataWorkerGcCounters");

    start_counters = reader.Read64();
    start_stats = reader.Read64();
    total_size = reader.Read64();

    uint64_t size_positions = reader.Read64();
    for (uint64_t index = 0; index < size_positions; ++index) {
        std::string name = reader.ReadString();
        uint64_t value = reader.Read64();
        // std::cout << "\t" << name << " = " << value << "\n";
        counter_positions[name] = value;
    }

    index_counters = start_counters / 8;
}

// ----------------------------------------------------------------------------
// Other counters

uint64_t MetadataOtherCounters::Size() {
    return BufferWriter::Size(2);
}

uint64_t MetadataOtherCounters::Initialize(uint64_t start) {
    start_bus_errors = start;
    start += BufferWriter::AllignToSizeCacheLine(static_cast<uint64_t>(common::idp::errorType::size) * sizeof(uint64_t));
    start_bus_requests = start;
    start += BufferWriter::AllignToSizeCacheLine(static_cast<uint64_t>(common::idp::requestType::size) * sizeof(uint64_t));
    return start;
}

void MetadataOtherCounters::WriteToBuffer(BufferWriter& writer) {
    writer.StartBlock("MetadataOtherCounters", Size());
    writer.Write64(start_bus_errors);
    writer.Write64(start_bus_requests);
}

void MetadataOtherCounters::ReadFromBuffer(BufferReader& reader) {
    reader.StartBlock("MetadataOtherCounters");
    start_bus_errors = reader.Read64();
    start_bus_requests = reader.Read64();
}

// ----------------------------------------------------------------------------
// MainFileData

void MainFileData::AddWorker(tSocketId socket_id, tCoreId core_id) {
    sockets.all_sockets[socket_id] = nullptr;
    workers.workers[core_id].socket_id = socket_id;
}

void MainFileData::AddWorkerGC(tSocketId socket_id, tCoreId core_id) {
    sockets.all_sockets[socket_id] = nullptr;
    workers_gc.workers[core_id].socket_id = socket_id;
}

std::string MainFileData::GetNameOfSocketFileName(tSocketId socket_id) {
    return SHARED_FILENAME_SOCKET_PREFIX + std::to_string(socket_id) + ".shm";
}

eResult MainFileData::FinalizeDataplane(bool useHugeMem) {
    uint64_t total_size = 0;
    total_size += sockets.Size();
    total_size += workers.Size();
    total_size += workers_gc.Size();
    total_size += metadata_workers.Size();
    total_size += metadata_workers_gc.Size();
    total_size += metadata_other_counters.Size();
    total_size = metadata_other_counters.Initialize(total_size);
    std::cout << "Total size finalized: " << total_size << "\n";

    main_buffer = CreateBufferInSharedMemory(SHARED_FILENAME_MAIN, total_size, useHugeMem);
    if (main_buffer == nullptr) {
        return eResult::errorInitSharedMemory;
    }
    BufferWriter writer(main_buffer, total_size);

    // Write sockects info
    sockets.WriteToBuffer(writer);

    // Prepare data for workers and workers_gc
    std::map<tSocketId, uint64_t> sizes_by_sockets;
    for (auto& iter : workers.workers) {
        iter.second.start_at_memory = sizes_by_sockets[iter.second.socket_id];
        sizes_by_sockets[iter.second.socket_id] += metadata_workers.total_size;
    }
    workers.WriteToBuffer(writer);
    for (auto& iter : workers_gc.workers) {
        iter.second.start_at_memory = sizes_by_sockets[iter.second.socket_id];
        sizes_by_sockets[iter.second.socket_id] += metadata_workers_gc.total_size;
    }
    workers_gc.WriteToBuffer(writer);

    // Create shared memory files by sockets
    for (auto& iter : sockets.all_sockets) {
        std::string fname = GetNameOfSocketFileName(iter.first);
        iter.second = CreateBufferInSharedMemory(fname.c_str(), sizes_by_sockets[iter.first], useHugeMem);
        if (iter.second == nullptr) {
            return eResult::errorInitSharedMemory;
        }
    }

    // Metadata counters
    metadata_workers.WriteToBuffer(writer);
    metadata_workers_gc.WriteToBuffer(writer);

    // Other counters
    metadata_other_counters.WriteToBuffer(writer);

    return eResult::success;
}

eResult  MainFileData::ReadFromDataplane(bool useHugeMem, bool open_sockets_file) {
    uint64_t size;
    main_buffer = OpenBufferInSharedMemory(SHARED_FILENAME_MAIN, false, useHugeMem, &size);
    // std::cout << "Size main file: " << size << ", buffer: " << main_buffer << "\n";
    if (main_buffer == nullptr) {
        return eResult::errorInitSharedMemory;
    }

    BufferReader reader(main_buffer, size);

    sockets.ReadFromBuffer(reader);
    workers.ReadFromBuffer(reader);
    workers_gc.ReadFromBuffer(reader);
    metadata_workers.ReadFromBuffer(reader);
    metadata_workers_gc.ReadFromBuffer(reader);
    metadata_other_counters.ReadFromBuffer(reader);

    if (open_sockets_file) {
        for (auto& iter : sockets.all_sockets) {
            std::string fname = GetNameOfSocketFileName(iter.first);
            iter.second = OpenBufferInSharedMemory(fname.c_str(), false, useHugeMem, &size);
            // std::cout << "Openned shared memory file: " << fname << ", size: " << size << "\n";
        }
    }

    return eResult::success;
}

void* MainFileData::GetStartBufferForWorker(tCoreId core_id) {
    const auto& info = workers.workers[core_id];
    return PtrAdd(sockets.all_sockets[info.socket_id], info.start_at_memory);
}

void* MainFileData::GetStartBufferForWorkerGC(tCoreId core_id) {
    const auto& info = workers_gc.workers[core_id];
    return PtrAdd(sockets.all_sockets[info.socket_id], info.start_at_memory);
}

uint64_t* MainFileData::GetStartBufferOtherCounters(uint64_t start) {
    return (uint64_t*)PtrAdd(main_buffer, start);
}

}  // namespace processes_data_exchange

}  // namespace common
