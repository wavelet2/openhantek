#include "utils/transferBuffer.h"

/// \brief Initializes the data array.
/// \param size Size of the data array.
TransferBuffer::TransferBuffer(unsigned int size) {
        this->array = new unsigned char[size];
        for(unsigned int index = 0; index < size; ++index)
                this->array[index] = 0;
        this->_size = size;
}

/// \brief Deletes the allocated data array.
TransferBuffer::~TransferBuffer() {
        delete[] this->array;
}

/// \brief Returns a pointer to the array data.
/// \return The internal data array.
unsigned char *TransferBuffer::data() {
        return this->array;
}

/// \brief Returns array element when using square brackets.
/// \return The array element.
unsigned char TransferBuffer::operator[](unsigned int index) {
        return this->array[index];
}

/// \brief Gets the size of the array.
/// \return The size of the command in bytes.
unsigned int TransferBuffer::size() const {
        return this->_size;
}
