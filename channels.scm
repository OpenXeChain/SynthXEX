;;; Lock the Guix repository to a specific commit.
;;; For reproducibility purposes in building SynthXEX.

(list (channel
       (name 'guix)
       (url "https://codeberg.org/guix/guix.git")
       (branch "master")
       (commit
        "95f8c22bbe9cf25ce492ef520f94a34f7ec3d160")
       (introduction
        (make-channel-introduction
         "9edb3f66fd807b096b48283debdcddccfea34bad"
         (openpgp-fingerprint
          "BBB0 2DDF 2CEA F6A8 0D1D  E643 A2A0 6DF2 A33A 54FA")))))
