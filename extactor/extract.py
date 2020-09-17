from dataclasses import dataclass


@dataclass
class MusicData:
    music_number: int
    stripped_data: bytes
    offset: int
    positions_table: int
    speed: int
    instrument_table_for_positions: int

    def format_to_C(self):
        data = ', '.join(f'0x{b:X}' for b in self.stripped_data)

        return f'''
uint8_t music_{self.music_number}_data[] = {{ {data} }};
constexpr MusicData music_{self.music_number} = {{
        music_{self.music_number}_data,
        0x{self.offset:X},
        0x{self.positions_table:X},
        {self.speed},
        0x{self.instrument_table_for_positions:X}
}};
'''


def get_word(data: bytes, address: int) -> int:
    return data[address] + data[address + 1] * 256


def extract_music_data(music_number: int, data: bytes, offset: int) -> MusicData:
    return MusicData(
        music_number=music_number,
        stripped_data=data[0x0313:],
        offset=offset + 0x0313,
        positions_table=get_word(data, 0x0001),
        speed=data[0x0012],
        instrument_table_for_positions=get_word(data, 0x007),
    )


def read_data() -> bytes:
    with open('music.dat', 'rb') as f:
        return f.read()


def main():
    data = read_data()

    music_1 = extract_music_data(1, data[0x0000:0x1300], 0xC000)
    print(music_1.format_to_C())

    music_2 = extract_music_data(2, data[0x1300:0x2D10], 0xD300)
    print(music_2.format_to_C())


if __name__ == '__main__':
    main()
