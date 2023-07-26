#include <stdbool.h>
#include "oc_assert.h"
#include "oc_helpers.h"

#ifndef OC_MAX_REPLAY_RECORDS
#define OC_MAX_REPLAY_RECORDS (20)
#endif

struct oc_replay_record
{
	uint64_t rx_ssn; /// most recent received SSN of client
	oc_string_t rx_kid; /// byte string holding the KID of the client
	oc_string_t rx_kid_ctx; /// byte string holding the KID context of the client. can be null
	bool in_use; /// whether this structure is in use & has valid data
};

// this is the list of synchronised clients
static struct oc_replay_record replay_records[OC_MAX_REPLAY_RECORDS] = {0};

// get the first available record
static struct oc_replay_record * get_empty_record()
{
	for (size_t i = 0; i < OC_MAX_REPLAY_RECORDS; ++i)
	{
		if (!replay_records[i].in_use)
			return replay_records + i;
	}
}

// find record with KID and CTX
static struct oc_replay_record * get_record(oc_string_t rx_kid, oc_string_t rx_kid_ctx)
{
	oc_assert(oc_byte_string_len(rx_kid) != 0);

	for (size_t i = 0; i < OC_MAX_REPLAY_RECORDS; ++i)
	{
		struct oc_replay_record *rec = replay_records + i;

		if (rec->in_use) {
			bool rx_kid_match = oc_byte_string_cmp(rx_kid, rec->rx_kid) == 0;
			bool null_contexts = oc_byte_string_len(rx_kid_ctx) == 0 && oc_byte_string_len(rec->rx_kid_ctx) == 0;
			bool contexts_match = oc_byte_string_cmp(rx_kid_ctx, rec->rx_kid_ctx);

			if(rx_kid_match && (null_contexts || contexts_match))
				return rec;
		}
	}
	return NULL;
}

// make record available for reuse
static void free_record(struct oc_replay_record *rec)
{
	// bounds check
	if (replay_records <= rec && rec < replay_records + OC_MAX_REPLAY_RECORDS)
	{
		rec->rx_ssn = 0;
		oc_free_string(&rec->rx_kid);
		oc_free_string(&rec->rx_kid_ctx);
		rec->in_use = false;
	}
}

// return true if SSN of device identified by KID & KID_CTX is within replay window
//    if it is, update SSN within replay record
// return false if no entry found, or if SSN is outside replay window
bool oc_replay_is_synchronized_client(uint64_t rx_ssn, oc_string_t rx_kid, oc_string_t rx_kid_ctx)
{
	struct oc_replay_record * rec = get_record(rx_kid, rx_kid_ctx);
	if (rec == NULL)
		return false;

	uint64_t window = 32; // TODO update this value with API

	if (rec->rx_ssn < rx_ssn && rx_ssn <= rec->rx_ssn + window)
	{
		// ssn is within window! save it

		// hmm, cannot guarantee messages are received in order...
		// what if you happen to receive SSN 32 followed by
		// non-replayed SSNs 28, 29, 30, 31... do you drop all these?
		rec->rx_ssn = rx_ssn;

		// better algorithm: also store a bitfield which keeps track 
		// of actually received messages outside of the window.

		// the bitwise position in this bitfield indicates whether 
		// a message was received with that message id

		// and you can use a saturating shift to modify the bitfield when the window shifts
		// 0th bit is always set as it represents the current SSN - makes math simpler
		// but maybe not strictly necessary

		/*
		
		for example:

		ssn = 1
		bitfield = 0b0000'0001


		rx 2, is valid. 2 - 1 = 1, so left shift bitfield by 1 and set 1 (for current ssn)

		ssn = 2
		bitfield = 0b0000'0011

		rx 2, thrown out because ssn is already 2 & bit 0 is set
		rx 1, thrown out because 2 - 1 = 1 and bit 1 is set

		rx 8, valid but has skipped some values. 8 - 2 = 6, so left shift bitfield by 6 and set 1

		ssn = 8
		bitfield = 0b1100'0001

		rx 7, invalid without bitfield but has not been seen before
		so we set bit 8 - 7 = 1 and accept it

		ssn = 8
		bitfield = 0b1100'0011

		rx 6, accept & set bit 8 - 6 = 2

		ssn = 8
		bitfield = 0b1100'0111

		rx 7 again, thrown out because bit 8 - 7 = 1 is set
		rx 2 again, thrown out because 8 - 2 = 6 and bit 6 is set!!

		rx 9, change ssn and left shift bitfield
		ssn = 9
		bitfield = 0b1000'1111

		have to assume that everything beyond what is tracked by the bitfield
		is out of the window, so ssns 1, 0 would be rejected
		*/


		/*

		PROBLEM: spec is ambiguous / contradicts with OSCORE
		paragraph 2731 of KNX specification is where the problem lies.
		if you implement that algorithm verbatim, you drop loads of valid packets

		if you look at oscore specification, it says that the 'default mechanism is an anti-replay
		sliding window (see section 4.1.2.6 of RFC6347).' Algorithm above implements said anti-replay
		sliding window, but setting the size of this window at run-time is non-trivial if we want
		to use the efficient bitfield
		
		*/

		return true;
	}
	else
	{
		return false;
	}
}

// update replay record if match found
// otherwise, create new replay record
void oc_replay_add_synchronised_client(uint64_t rx_ssn, oc_string_t rx_kid, oc_string_t rx_kid_ctx)
{


}

