#pragma once

//////////////////////////////////////////////////////////////////////////////
///
/// \brief A simple array with a fixed size allocated on the heap
class TransferBuffer {
        public:
                TransferBuffer(unsigned int size);
                ~TransferBuffer();

                unsigned char *data();
                unsigned char operator[](unsigned int index);

                unsigned int size() const;

        protected:
                unsigned char *array; ///< Pointer to the array holding the data
                unsigned int _size; ///< Size of the array (Number of variables of type T)
};
