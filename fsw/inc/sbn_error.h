#ifndef SBN_ERROR_H
#define SBN_ERROR_H

/* General status codes */
#define SBN_FINISHED        1 /* a flag to finish the current operation, this is >0 because it is not an error */
#define SBN_SUCCESS         0
#define SBN_ERROR           -1
#define SBN_IF_EMPTY        -2
#define SBN_NOT_IMPLEMENTED -3
#define SBN_NO_PEER         -4
#define SBN_INVALID_VERSION -5
#define SBN_NULL_OUTLET     -6

#endif /* SBN_ERROR_H */
