#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>

namespace pm
{

	template< typename T, size_t Capacity >
	class ring_buffer
	{
	public:
		ring_buffer( ) : write_pos_( 0 ), read_pos_( 0 ) { }

		// push samples into the buffer
		void push( const T* data, size_t count )
		{
			size_t write_pos = write_pos_.load( std::memory_order_relaxed );

			for ( size_t i = 0; i < count; ++i ) {
				buffer_[ ( write_pos + i ) % Capacity ] = data[ i ];
			}

			write_pos_.store( ( write_pos + count ) % Capacity, std::memory_order_release );
		}

		// pop samples from the buffer
		size_t pop( T* dest, size_t count )
		{
			size_t avail   = available( );
			size_t to_read = std::min( count, avail );

			size_t read_pos = read_pos_.load( std::memory_order_relaxed );

			for ( size_t i = 0; i < to_read; ++i ) {
				dest[ i ] = buffer_[ ( read_pos + i ) % Capacity ];
			}

			read_pos_.store( ( read_pos + to_read ) % Capacity, std::memory_order_release );
			return to_read;
		}

		// peek at samples without consuming them
		size_t peek( T* dest, size_t count ) const
		{
			size_t avail   = available( );
			size_t to_read = std::min( count, avail );

			size_t read_pos = read_pos_.load( std::memory_order_acquire );

			for ( size_t i = 0; i < to_read; ++i ) {
				dest[ i ] = buffer_[ ( read_pos + i ) % Capacity ];
			}

			return to_read;
		}

		// get the most recent N samples (for visualization)
		size_t peek_recent( T* dest, size_t count ) const
		{
			size_t avail = available( );
			if ( avail == 0 )
				return 0;

			size_t to_read   = std::min( count, avail );
			size_t write_pos = write_pos_.load( std::memory_order_acquire );
			size_t start_pos = ( write_pos - to_read + Capacity ) % Capacity;

			for ( size_t i = 0; i < to_read; ++i ) {
				dest[ i ] = buffer_[ ( start_pos + i ) % Capacity ];
			}

			return to_read;
		}

		// number of samples available to read
		size_t available( ) const
		{
			size_t write_pos = write_pos_.load( std::memory_order_acquire );
			size_t read_pos  = read_pos_.load( std::memory_order_acquire );

			if ( write_pos >= read_pos ) {
				return write_pos - read_pos;
			} else {
				return Capacity - read_pos + write_pos;
			}
		}

		// total capacity
		constexpr size_t capacity( ) const
		{
			return Capacity;
		}

		// clear the buffer
		void clear( )
		{
			read_pos_.store( 0, std::memory_order_release );
			write_pos_.store( 0, std::memory_order_release );
		}

	private:
		T buffer_[ Capacity ];
		std::atomic< size_t > write_pos_;
		std::atomic< size_t > read_pos_;
	};

} // namespace pm
