class STM32CRC:
    def __init__(self, poly_value=0x4C11DB7, init_value=0xFFFFFFFF):
        self._poly = poly_value
        self._init = init_value

    def poly_value(self):
        return self._poly

    def init_value(self):
        return self._init

    def calculate(self, data):
        crc = self._init

        for i in range(len(data) // 4):
            word = self._bytes_to_word(data[i*4:(i*4)+4])
            crc = self._calc32(crc, word)

        remainder = len(data) % 4
        if remainder != 0:
            crc = self._calc32(crc, self._bytes_to_word(data[-remainder:]))

        return crc

    def _calc32(self, previous, word):
        crc = previous ^ word
        for i in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) & 0xFFFFFFFF) ^ self._poly
            else:
                crc = (crc << 1) & 0xFFFFFFFF
        return crc

    # converts to little endian, as it's compatible with STM32 CRC endianess
    @staticmethod
    def _bytes_to_word(bytes):
        word = 0
        for i in range(len(bytes)):
            word |= (bytes[i] << (8 * i))

        return word & 0xFFFFFFFF