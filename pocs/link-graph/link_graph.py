#!/usr/bin/env python
#
#    LinkGraph.py: this file is part of the elfix package
#    Copyright (C) 2011  Anthony G. Basile
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re
import portage

class LinkGraph:

    def __init__(self):
        """ Put all the NEEDED.ELF.2 files for all installed packages
        into a dictionary of the form

            { pkg : line_from_NEEDED.ELF.2, ... }

        where the line has the following form:

           echo "${arch:3};${obj};${soname};${rpath};${needed}" >> \
               "${PORTAGE_BUILDDIR}"/build-info/NEEDED.ELF.2

        See /usr/lib/portage/bin/misc-functions.sh ~line 520
        """

        vardb = portage.db[portage.root]["vartree"].dbapi

        self.pkgs = []
        self.pkgs_needed = {}

        for pkg in vardb.cpv_all():
            needed = vardb.aux_get(pkg, ['NEEDED.ELF.2'])[0].strip()
            if needed: # Some packages have no NEEDED.ELF.2
                self.pkgs.append(pkg)
                for line in re.split('\n', needed):
                    self.pkgs_needed.setdefault(pkg,[]).append(re.split(';', line))


    def get_object_needed(self):
        """ Return object_needed dictionary which has structure

            {
                abi1 : { full_path_to_ELF_object : [ soname1, soname2, ... ], ... },
                abi2 : { full_path_to_ELF_object : [ soname1, soname2, ... ], ... },
                ....
            }

        Here the sonames were obtained from the ELF object by scanelf -nm
        (like readelf -d) during emerge.
        """
        object_needed = {}

        for pkg in self.pkgs:
            for link in self.pkgs_needed[pkg]:
                abi = link[0]
                elf = link[1]
                sonames = re.split(',', link[4])
                object_needed.setdefault(abi,{}).update({elf:sonames})

        return object_needed


    def get_libraries(self):
        """ Return library2soname dictionary which has structure

            { full_path_to_library : (soname, abi), ... }

        and its inverse which has structure

            { (soname, abi) : full_path_to_library, ... }
        """
        library2soname = {}
        soname2library = {}

        for pkg in self.pkgs:
            for link in self.pkgs_needed[pkg]:
                abi = link[0]
                elf = link[1]
                soname = link[2]
                if soname:                               #no soname => executable
                    library2soname[elf] = (soname,abi)
                    soname2library[(soname,abi)] = elf

        return ( library2soname, soname2library )


    def get_soname_needed(self, object_needed, library2soname ):
        """ Return soname_needed dictionary which has structure:

            {
                abi1: { soname: [ soname1, soname2, ... ], .... },
                abi2: { soname: [ soname1, soname2, ... ], .... },
            }

        Here the soname1, soname2,... were obtained from soname's corresponding
        ELF object by scanelf -n during emerge.
        """
        soname_needed = {}

        for abi in object_needed:
            for elf in object_needed[abi]:
                try:
                    (soname, abi_check) = library2soname[elf]
                    assert abi == abi_check # We should get the same abi associated with the soname
                    soname_needed.setdefault(abi,{}).update({soname:object_needed[abi][elf]})
                except KeyError:
                    continue  # no soname, its probably an executable

        return soname_needed


    def expand_linkings(self, object_needed, soname2library):
        """ Expands the object_needed dictionary which has structure

            {
                abi1 : { full_path_to_ELF_object : [ soname1, soname2, ... ], ... },
                abi2 : { full_path_to_ELF_object : [ soname1, soname2, ... ], ... },
                ....
            }

        such that the soname's are traced all the way to the end of
        the link chain.  Here the sonames should be the same as those
        obtained from the ELF object by ldd.
        """
        for abi in object_needed:
            for elf in object_needed[abi]:
                while True:
                    found_new_soname = False
                    for so in object_needed[abi][elf]:                              # For all the first links ...
                        try:
                            for sn in object_needed[abi][soname2library[(so,abi)]]: # go to the next links ...
                                if sn in object_needed[abi][elf]:                   # skip if already included ...
                                    continue
                                if not (sn,abi) in soname2library:                  # skip if vdso ...
                                    continue

                                # This appends to the object_needed and soname_needed lists.  No copy was
                                # done so its the same lists in memory for both, and its modified for both.

                                object_needed[abi][elf].append(sn)                  # otherwise collapse it back into
                                found_new_soname = True                             # first links of the chain.

                        except KeyError:                 # Not all nodes in the chain have a next node
                            continue

                    if not found_new_soname:             # We're done, that last iteration found
                        break                            # no new nodes


    def get_object_reverse_linkings(self, object_linkings):
        """ Return object_reverse_linkings dictionary which has structure

            {
                abi1 : { soname : [ path_to_elf1, path_to_elf2, ... ], ... },
                abi2 : { soname : [ path_to_elf3, path_to_elf4, ... ], ... },
                ....
            }
        """
        object_reverse_linkings = {}

        for abi in object_linkings:
            for elf in object_linkings[abi]:
                for soname in object_linkings[abi][elf]:
                    object_reverse_linkings.setdefault(abi,{}).setdefault(soname,[]).append(elf)

        return object_reverse_linkings


    def get_graph(self):
        """ Generate the full forward and reverse links using the above functions """

        # After get_object_needed() and get_soname_needed(), both object_linkings and
        # soname_linkings are only one step into the entire link chain.

        object_linkings = self.get_object_needed()
        ( library2soname, soname2library ) = self.get_libraries()
        soname_linkings = self.get_soname_needed( object_linkings, library2soname )

        # After the appending in expand_linkings(), forward_linkings and soname_linkings
        # have been extended through the entire chain of linking.  expand_linkings() is
        # a "side-effect" function, so we note it here.
        self.expand_linkings( soname_linkings, soname2library )
        object_reverse_linkings = self.get_object_reverse_linkings( object_linkings )

        return ( object_linkings, object_reverse_linkings, library2soname, soname2library )

