#include "demake/zone_resources.hpp"

#ifdef __3DS__
#include <3ds.h>
#else
#include <cstdlib>
#endif

#include <cstdio>
#include <cstring>

namespace demake {
namespace {

constexpr std::size_t kHeaderBytes = 12;
constexpr std::size_t kRecordBytes = 43;
constexpr std::uint16_t kFormatVersion = 1;
constexpr std::size_t kMaximumBoxesPerZone = 256;

unsigned zoneIndex(Zone zone) {
    return static_cast<unsigned>(zone);
}

const char* zonePath(Zone zone) {
#ifdef __3DS__
    constexpr const char* kInteriorPath = "romfs:/zones/interior.bin";
    constexpr const char* kVistaPath = "romfs:/zones/vista.bin";
    constexpr const char* kArenaPath = "romfs:/zones/arena.bin";
    constexpr const char* kFieldPath = "romfs:/zones/field.bin";
#else
    constexpr const char* kInteriorPath = "romfs/zones/interior.bin";
    constexpr const char* kVistaPath = "romfs/zones/vista.bin";
    constexpr const char* kArenaPath = "romfs/zones/arena.bin";
    constexpr const char* kFieldPath = "romfs/zones/field.bin";
#endif
    switch (zone) {
        case Zone::Interior: return kInteriorPath;
        case Zone::Vista: return kVistaPath;
        case Zone::Arena: return kArenaPath;
        case Zone::Field: return kFieldPath;
    }
    return nullptr;
}

void* allocateZoneMemory(std::size_t bytes) {
#ifdef __3DS__
    return linearAlloc(bytes);
#else
    return std::malloc(bytes);
#endif
}

void freeZoneMemory(void* memory) {
#ifdef __3DS__
    linearFree(memory);
#else
    std::free(memory);
#endif
}

std::uint16_t readU16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0]) |
           static_cast<std::uint16_t>(data[1] << 8U);
}

std::uint32_t readU32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8U) |
           (static_cast<std::uint32_t>(data[2]) << 16U) |
           (static_cast<std::uint32_t>(data[3]) << 24U);
}

float readFloat(const std::uint8_t* data) {
    float value = 0.0f;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

bool readExact(std::FILE* file, void* destination, std::size_t bytes) {
    return std::fread(destination, 1, bytes, file) == bytes;
}

} // namespace

bool ZoneResources::sync(std::uint8_t requested_mask) {
    constexpr std::uint8_t kZoneMask = 0x0FU;
    if ((requested_mask & ~kZoneMask) != 0U) {
        return false;
    }
    if (requested_mask == loaded_mask_) {
        return true;
    }

    const std::uint8_t previous_mask = loaded_mask_;
    for (unsigned index = 0; index < slots_.size(); ++index) {
        const std::uint8_t bit = static_cast<std::uint8_t>(1U << index);
        if ((requested_mask & bit) != 0U && (loaded_mask_ & bit) == 0U) {
            if (!load(static_cast<Zone>(index))) {
                for (unsigned rollback = 0; rollback < slots_.size(); ++rollback) {
                    const std::uint8_t rollback_bit = static_cast<std::uint8_t>(1U << rollback);
                    if ((previous_mask & rollback_bit) == 0U &&
                        (loaded_mask_ & rollback_bit) != 0U) {
                        unload(static_cast<Zone>(rollback));
                    }
                }
                return false;
            }
        }
    }
    for (unsigned index = 0; index < slots_.size(); ++index) {
        const std::uint8_t bit = static_cast<std::uint8_t>(1U << index);
        if ((requested_mask & bit) == 0U && (loaded_mask_ & bit) != 0U) {
            unload(static_cast<Zone>(index));
        }
    }
    return loaded_mask_ == requested_mask;
}

bool ZoneResources::load(Zone zone) {
    const unsigned index = zoneIndex(zone);
    if (index >= slots_.size() || slots_[index].boxes) {
        return index < slots_.size();
    }
    const char* path = zonePath(zone);
    std::FILE* file = path ? std::fopen(path, "rb") : nullptr;
    if (!file) {
        return false;
    }

    std::uint8_t header[kHeaderBytes]{};
    if (!readExact(file, header, sizeof(header)) ||
        std::memcmp(header, "ASZN", 4) != 0 ||
        readU16(header + 4) != kFormatVersion ||
        readU16(header + 6) != kRecordBytes) {
        std::fclose(file);
        return false;
    }
    const std::size_t count = readU32(header + 8);
    if (count == 0 || count > kMaximumBoxesPerZone) {
        std::fclose(file);
        return false;
    }

    SceneBox* boxes = static_cast<SceneBox*>(allocateZoneMemory(count * sizeof(SceneBox)));
    if (!boxes) {
        std::fclose(file);
        return false;
    }
    bool valid = true;
    for (std::size_t index_box = 0; index_box < count; ++index_box) {
        std::uint8_t record[kRecordBytes]{};
        if (!readExact(file, record, sizeof(record))) {
            valid = false;
            break;
        }
        SceneBox& box = boxes[index_box];
        float* values[] = {
            &box.x, &box.y, &box.z, &box.sx, &box.sy, &box.sz,
            &box.rotation_y, &box.red, &box.green, &box.blue,
        };
        for (std::size_t value = 0; value < 10; ++value) {
            *values[value] = readFloat(record + value * sizeof(float));
        }
        box.cell_x = static_cast<std::int8_t>(record[40]);
        box.cell_z = static_cast<std::int8_t>(record[41]);
        box.always = record[42] != 0;
    }
    if (valid && std::fgetc(file) != EOF) {
        valid = false;
    }
    std::fclose(file);
    if (!valid) {
        freeZoneMemory(boxes);
        return false;
    }

    Slot& slot = slots_[index];
    slot.boxes = boxes;
    slot.count = count;
    slot.bytes = static_cast<std::uint32_t>(count * sizeof(SceneBox));
    resident_bytes_ += slot.bytes;
    loaded_mask_ |= static_cast<std::uint8_t>(1U << index);
    ++load_count_;
    return true;
}

void ZoneResources::unload(Zone zone) {
    const unsigned index = zoneIndex(zone);
    if (index >= slots_.size() || !slots_[index].boxes) {
        return;
    }
    Slot& slot = slots_[index];
    freeZoneMemory(slot.boxes);
    slot.boxes = nullptr;
    slot.count = 0;
    resident_bytes_ = resident_bytes_ > slot.bytes ? resident_bytes_ - slot.bytes : 0;
    slot.bytes = 0;
    loaded_mask_ &= static_cast<std::uint8_t>(~(1U << index));
    ++unload_count_;
}

const SceneBox* ZoneResources::boxes(Zone zone, std::size_t& count) const {
    const unsigned index = zoneIndex(zone);
    if (index >= slots_.size()) {
        count = 0;
        return nullptr;
    }
    count = slots_[index].count;
    return slots_[index].boxes;
}

void ZoneResources::shutdown() {
    for (unsigned index = 0; index < slots_.size(); ++index) {
        unload(static_cast<Zone>(index));
    }
}

} // namespace demake
