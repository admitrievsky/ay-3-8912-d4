#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H

struct MusicData {
    const uint8_t* data;
    uint16_t offset;
    uint16_t positions_table;
    uint8_t speed;
    uint16_t instrument_table_for_positions;

    uint8_t read_byte(uint16_t address) const {
        return pgm_read_byte_near(data + address - offset);
    }

    uint16_t read_word(uint16_t address) const {
        return pgm_read_word_near(data + address - offset);
    }
};

MusicData get_music_data_1();
MusicData get_music_data_2();

#endif
