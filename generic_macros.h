#ifndef SOUNDREX_GENERIC_MACROS
#define SOUNDREX_GENERIC_MACROS

// TODO: Implement using __VA_OPT__ rather than non-standard GCC/CLANG macro, once that becomes available
#define NUMARGS(...) NUMARGS_I(0, ##__VA_ARGS__,9,8,7,6,5,4,3,2,1,0)
#define NUMARGS_I(_,_9,_8,_7,_6,_5,_4,_3,_2,_1,X_,...) X_

#define CONCAT_I(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_I(X, Y)

#define STRING_I(X) #X
#define STRING(X) STRING_I(X)

#define FOLDR(INDIV_, JOINER_, ...) CONCAT(FOLDR_, NUMARGS(__VA_ARGS__))(INDIV_, JOINER_, __VA_ARGS__)
#define FOLDR_0(INDIV_, JOINER_, ...)
#define FOLDR_1(INDIV_, JOINER_, X, ...) INDIV_(X)
#define FOLDR_2(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_1(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_3(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_2(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_4(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_3(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_5(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_4(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_6(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_5(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_7(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_6(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_8(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_7(INDIV_, JOINER_, __VA_ARGS__))
#define FOLDR_9(INDIV_, JOINER_, X, ...) JOINER_(INDIV_(X), FOLDR_8(INDIV_, JOINER_, __VA_ARGS__))

#define IDEN(...) __VA_ARGS__
#define MAP(NAME_, ...) FOLDR(NAME_, IDEN, __VA_ARGS__)

#define CAR(X, ...) X
#define CDR(X, ...) __VA_ARGS__

#endif // SOUNDREX_GENERIC_MACROS
