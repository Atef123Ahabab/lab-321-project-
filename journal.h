#ifndef JOURNAL_H
#define JOURNAL_H

// Create a new file (logs changes to journal)
int create(const char *filename);

// Install journal transactions to the file system
int install(void);

#endif // JOURNAL_H
