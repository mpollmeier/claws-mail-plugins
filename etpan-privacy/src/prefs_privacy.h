#ifndef PREFS_PRIVACY_H

#define PREFS_PRIVACY_H

#define ETPANPRIVACY_RC "etpanprivacyrc"
#define PREFS_PRIVACY_DEFAULT_CA_DIR "smime/CA"
#define PREFS_PRIVACY_DEFAULT_CERT_DIR "smime/cert"
#define PREFS_PRIVACY_DEFAULT_PRIVATE_KEYS_DIR "smime/keys"

void etpan_privacy_prefs_init(void);
void etpan_privacy_prefs_done(void);

#endif
