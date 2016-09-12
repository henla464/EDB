#include <Arduino.h>
#include "EDB.h"

/**************************************************/
// private functions

// low level byte write
void EDB::edbWrite(unsigned long ee, const byte* p, unsigned int recsize)
{
  for (unsigned int i = 0; i < recsize; i++)
    _write_byte(ee++, *p++);
}

// low level byte read
void EDB::edbRead(unsigned long ee, byte* p, unsigned int recsize)
{  
  for (unsigned int i = 0; i < recsize; i++)
    *p++ = _read_byte(ee++);
}

// writes EDB_Header
void EDB::writeHead()
{
  edbWrite(EDB_head_ptr, EDB_REC EDB_head, (unsigned long)sizeof(EDB_Header));
}

// reads EDB_Header
void EDB::readHead()
{
  edbRead(EDB_head_ptr, EDB_REC EDB_head, (unsigned long)sizeof(EDB_Header));
}

/**************************************************/
// public functions

EDB::EDB(EDB_Write_Handler *w, EDB_Read_Handler *r)
{
  _write_byte = w;
  _read_byte = r;
}

// creates a new table and sets header values 
EDB_Status EDB::create(unsigned long head_ptr, unsigned long tablesize, unsigned int recsize)
{
  EDB_head_ptr = head_ptr;
  EDB_table_ptr = sizeof(EDB_Header) + EDB_head_ptr;
  EDB_head.n_recs = 0;
  EDB_head.rec_size = recsize;
  EDB_head.table_size = tablesize;
  writeHead();
  return EDB_OK;
}

// reads an existing edb header at a given recno and sets header values
EDB_Status EDB::open(unsigned long head_ptr, unsigned long tablesize, unsigned int recsize)
{
  EDB_head_ptr = head_ptr;
  EDB_table_ptr = sizeof(EDB_Header) + EDB_head_ptr;
  EDB_head.n_recs = 0;
  EDB_head.rec_size = recsize;
  EDB_head.table_size = tablesize;
  readHead();
  if (EDB_head.rec_size > recsize ||   EDB_head.table_size > tablesize)
  {
     // Corrupt file probably, or sizes changed
     EDB_head.n_recs = 0;
     EDB_head.rec_size = recsize;
     EDB_head.table_size = tablesize;
     return EDB_CORRUPT;
  }
  return EDB_OK;
}

// writes a record to a given recno
EDB_Status EDB::writeRec(unsigned long recno, const EDB_Rec rec)
{
  edbWrite(EDB_table_ptr + ((recno - 1) * EDB_head.rec_size), rec, EDB_head.rec_size);
  return EDB_OK;
}

// reads a record from a given recno
EDB_Status EDB::readRec(unsigned long recno, EDB_Rec rec)
{
  if (recno < 1 || recno > EDB_head.n_recs) {
	return EDB_OUT_OF_RANGE;
  }
  edbRead(EDB_table_ptr + ((recno - 1) * EDB_head.rec_size), rec, EDB_head.rec_size);
  return EDB_OK;
}

// Deletes a record at a given recno
// Becomes more inefficient as you the record set increases and you delete records 
// early in the record queue.
EDB_Status EDB::deleteRec(unsigned long recno)
{
  if (recno < 0 || recno > EDB_head.n_recs) return  EDB_OUT_OF_RANGE;
  EDB_Rec rec = (byte*)malloc(EDB_head.rec_size);
  for (unsigned long i = recno + 1; i <= EDB_head.n_recs; i++)
  {
    readRec(i, rec);
    writeRec(i - 1, rec);
  }  
  free(rec);
  EDB_head.n_recs--;
  writeHead();
  return EDB_OK;
}

// Inserts a record at a given recno, increasing all following records' recno by 1.
// This function becomes increasingly inefficient as it's currently implemented and 
// is the slowest way to add a record.
EDB_Status EDB::insertRec(unsigned long recno, EDB_Rec rec)
{
  if (count() == limit()) return EDB_TABLE_FULL;
  if (count() > 0 && (recno < 0 || recno > EDB_head.n_recs)) return EDB_OUT_OF_RANGE;
  if (count() == 0 && recno == 1) return appendRec(rec);

  EDB_Rec buf = (byte*)malloc(EDB_head.rec_size);
  for (unsigned long i = EDB_head.n_recs; i >= recno; i--)
  {
    readRec(i, buf);
    writeRec(i + 1, buf);
  }
  free(buf);
  writeRec(recno, rec);  
  EDB_head.n_recs++;
  writeHead();
  return EDB_OK;
}

// Updates a record at a given recno
EDB_Status EDB::updateRec(unsigned long recno, EDB_Rec rec)
{
  if (recno < 0 || recno > EDB_head.n_recs) return EDB_OUT_OF_RANGE;
  writeRec(recno, rec);  
  return EDB_OK;
}
