;;; This file is the git repo bundled package definition of SynthXEX for GNU Guix.
;;; To build natively (reproducibly), run "guix time-machine -C channels.scm -- build -f synthxex.scm" in the project root.
;;; To cross compile (reproducibly), run "guix time-machine -C channels.scm -- build -f synthxex.scm --target=<target triplet>" in the project root.
;;; To install, swap "build" for "package" in the above commands
;;;
;;; Copyright (c) 2025 Aiden Isik
;;;
;;; This program is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU Affero General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU Affero General Public License for more details.
;;;
;;; You should have received a copy of the GNU Affero General Public License
;;; along with this program.  If not, see <https://www.gnu.org/licenses/>.


;;; Specify the modules we need for this package definition
(define-module (synthxex)
  #:use-module (guix gexp)
  #:use-module (guix packages)
  #:use-module (guix licenses)
  #:use-module (guix build-system cmake)
  #:use-module (git)
  #:use-module (gnu packages version-control)
  #:use-module (ice-9 regex))


;;; Hardcoded fallback version in case we can't rely on git
(define synthxex-fallback-version "v0.0.5")


;;; Determine the version of SynthXEX we are building
;;; Returns: version string of SynthXEX being built
(define (get-synthxex-version)
  (cond
   ((< (libgit2-init!) 0)
    (display "Warning: failed to initialise libgit2. Using fallback version number.\n")
    synthxex-fallback-version)

   (else
    (let ((synthxex-dir (dirname (current-filename))))
      (cond
       ((eq? (openable-repository? synthxex-dir) #f)
	(display "Note: not a git repository. Using fallback version number.\n")
	(libgit2-shutdown!)
	synthxex-fallback-version)

       (else
	(let ((synthxex-repo (repository-open synthxex-dir)))
	  (cond
	   ((repository-empty? synthxex-repo)
	    (display "Warning: git repository is empty. Using fallback version number.\n")
	    (libgit2-shutdown!)
	    synthxex-fallback-version)

	   ((repository-shallow? synthxex-repo)
	    (display "Warning: git repository is shalllow. Using fallback version number.\n")
	    (libgit2-shutdown!)
	    synthxex-fallback-version)

	   ((repository-bare? synthxex-repo)
	    (display "Warning: git repository is bare. Using fallback version number.\n")
	    (libgit2-shutdown!)
	    synthxex-fallback-version)

	   (else
	    (let* ((synthxex-desc-opts (make-describe-options #:strategy 'tags)) ; Equivalent to git describe --tags
		   (synthxex-description (describe-workdir synthxex-repo synthxex-desc-opts))
		   (synthxex-format-opts (make-describe-format-options #:dirty-suffix "-dirty")) ; Equivalent to git describe --dirty
		   (synthxex-version-str (describe-format synthxex-description synthxex-format-opts)))
	      (libgit2-shutdown!) ; Whatever the result of the regex comparison, we're done with git. Clean up here.

	      (cond ; Check if the version string returned is valid. If it isn't, use the fallback.
	       ((eq? (string-match "^v[0-9]+\\.[0-9]+\\.[0-9]+(-[0-9]+-g[0-9a-f]+(-dirty)?)?$" synthxex-version-str) #f)
		(display "Warning: version string is invalid. Using fallback version number.\n")
		synthxex-fallback-version)

	       (else
		synthxex-version-str))))))))))))


;;; Define the SynthXEX package
(define-public synthxex
  (package
   (name "synthxex")
   (version (get-synthxex-version))
   (source (local-file (dirname (current-filename)) #:recursive? #t))
   (build-system cmake-build-system)
   (arguments
    '(#:build-type "Debug" ; This is the version of the package definition bundled in the git repo, so we should build a debug version
      #:tests? #f)) ; We don't have a test suite
   (native-inputs
    (list git)) ; Needed for version number generation by cmake
   (synopsis "XEX2 generator for the Xbox 360 games console")
   (description "SynthXEX is an XEX2 generator, the final component of development toolchains for the Xbox 360 games console.")
   (home-page "https://github.com/OpenXeChain/SynthXEX")
   (license agpl3+)))

synthxex
