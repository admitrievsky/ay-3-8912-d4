#ifndef WIN_IMPLEMENTATION_MUSIC_DATA_H
#define WIN_IMPLEMENTATION_MUSIC_DATA_H

struct MusicData {
    uint8_t* data;
    uint16_t offset;
    uint16_t positions_table;
    uint8_t speed;
    uint16_t instrument_table_for_positions;

    uint8_t read_byte(int address) const {
        return data[address - offset];
    }

    uint16_t read_word(int address) const {
        return data[address - offset] + (data[address - offset + 1] << 8u);
    }
};

MusicData get_music_data_1();
MusicData get_music_data_2();

#endif //WIN_IMPLEMENTATION_MUSIC_DATA_H
