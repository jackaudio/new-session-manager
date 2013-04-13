//
//  LADSPAInfo.C - Class for indexing information on LADSPA Plugins
//
//  Copyleft (C) 2002  Mike Rawes <myk@waxfrenzy.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

// #include <config.h>

#include <vector>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>

#include <ladspa.h>

#define HAVE_LIBLRDF 1
#ifdef HAVE_LIBLRDF
#include <lrdf.h>
#endif

#include "LADSPAInfo.h"

using namespace std;

LADSPAInfo::LADSPAInfo(bool override,
                       const char *path_list)
{
	if (strlen(path_list) > 0) {
		m_ExtraPaths = strdup(path_list);
	} else {
		m_ExtraPaths = NULL;
	}
	m_LADSPAPathOverride = override;

	RescanPlugins();
}

LADSPAInfo::~LADSPAInfo()
{
	CleanUp();
}

void
LADSPAInfo::RescanPlugins(void)
{
// Clear out what we've got
	CleanUp();

	if (!m_LADSPAPathOverride) {
	// Get $LADPSA_PATH, if available
		char *ladspa_path = getenv("LADSPA_PATH");
		if (ladspa_path) {
			ScanPathList(ladspa_path, &LADSPAInfo::ExaminePluginLibrary);

		} else {

			cerr << "WARNING: LADSPA_PATH environment variable not set" << endl;
			cerr << "         Assuming /usr/lib/ladspa:/usr/local/lib/ladspa" << endl;

			ScanPathList("/usr/lib/ladspa:/usr/local/lib/ladspa", &LADSPAInfo::ExaminePluginLibrary);
		}
	}

// Check any supplied extra paths
	if (m_ExtraPaths) {
		ScanPathList(m_ExtraPaths, &LADSPAInfo::ExaminePluginLibrary);
	}

// Do we have any plugins now?
	if (m_Plugins.size() == 0) {
		cerr << "WARNING: No plugins found" << endl;
	} else {
		cerr << m_Plugins.size() << " plugins found in " << m_Libraries.size() << " libraries" << endl;

#ifdef HAVE_LIBLRDF
	// Got some plugins. Now search for RDF data
		lrdf_init();

		char *rdf_path = getenv("LADSPA_RDF_PATH");

		if (rdf_path) {
		// Examine rdf info
			ScanPathList(rdf_path, &LADSPAInfo::ExamineRDFFile);

		} else {
			cerr << "WARNING: LADSPA_RDF_PATH environment variable not set" << endl;
			cerr << "         Assuming /usr/share/ladspa/rdf:/usr/local/share/ladspa/rdf" << endl;

		// Examine rdf info
			ScanPathList("/usr/share/ladspa/rdf:/usr/local/share/ladspa/rdf", &LADSPAInfo::ExamineRDFFile);
		}
		MetadataRDFDescend(LADSPA_BASE "Plugin", 0);

	// See which plugins were not added to an rdf group, and add them
	// all into the top level 'LADSPA' one
		list<unsigned long> rdf_p;

	// Get indices of plugins added to groups
		for (vector<RDFURIInfo>::iterator ri = m_RDFURIs.begin(); ri != m_RDFURIs.end(); ri++) {
			rdf_p.insert(rdf_p.begin(), ri->Plugins.begin(), ri->Plugins.end());
		}

	// Add all uncategorized plugins to top level group, subclassed by their
	// library's basename.
		rdf_p.unique();
		rdf_p.sort();
		unsigned long last_p = 0;
		for (list<unsigned long>::iterator p = rdf_p.begin(); p != rdf_p.end(); p++) {
			if ((*p - last_p) > 1) {
				for (unsigned long i = last_p + 1; i < *p; i++) {
				// URI 0 is top-level "LADSPA" group
					m_RDFURIs[0].Plugins.push_back(i);
				}
			}
			last_p = *p;
		}
		while (++last_p < m_Plugins.size()) {
		// URI 0 is top-level "LADSPA" group
			m_RDFURIs[0].Plugins.push_back(last_p);
		}

		lrdf_cleanup();
#else
	// No RDF. Add all plugins to top-level group
		RDFURIInfo ri;

		ri.URI = "";
		ri.Label = "LADSPA";

		m_RDFURIs.push_back(ri);
		m_RDFLabelLookup["LADSPA"] = 0;

		for (unsigned long i = 0; i < m_Plugins.size(); i++) {
		// Add plugin index
			m_RDFURIs[0].Plugins.push_back(i);
		}
#endif
	}
}

void
LADSPAInfo::UnloadAllLibraries(void)
{
// Blank descriptors
	for (vector<PluginInfo>::iterator i = m_Plugins.begin();
		i != m_Plugins.end(); i++) {
		if (i->Descriptor) i->Descriptor = NULL;
	}
// Unload DLLs,
	for (vector<LibraryInfo>::iterator i = m_Libraries.begin();
		i != m_Libraries.end(); i++) {
		if (i->Handle) {
			dlclose(i->Handle);
			i->Handle = NULL;
		}
		i->RefCount = 0;
	}
}

const LADSPA_Descriptor *
LADSPAInfo::GetDescriptorByID(unsigned long unique_id)
{
	if (m_IDLookup.find(unique_id) == m_IDLookup.end()) {
		cerr << "LADSPA Plugin ID " << unique_id << " not found!" << endl;
		return NULL;
	}

// Got plugin index
	unsigned long plugin_index = m_IDLookup[unique_id];

	PluginInfo *pi = &(m_Plugins[plugin_index]);
	LibraryInfo *li = &(m_Libraries[pi->LibraryIndex]);

	if (!(pi->Descriptor)) {
		LADSPA_Descriptor_Function desc_func = GetDescriptorFunctionForLibrary(pi->LibraryIndex);
		if (desc_func) pi->Descriptor = desc_func(pi->Index);
	}

	if (pi->Descriptor) {

	// Success, so increment ref counter for library
		li->RefCount++;
	}

	return pi->Descriptor;
}

void
LADSPAInfo::DiscardDescriptorByID(unsigned long unique_id)
{
	if (m_IDLookup.find(unique_id) == m_IDLookup.end()) {
		cerr << "LADSPA Plugin ID " << unique_id << " not found!" << endl;
	} else {

	// Get plugin index
		unsigned long plugin_index = m_IDLookup[unique_id];

		PluginInfo *pi = &(m_Plugins[plugin_index]);
		LibraryInfo *li = &(m_Libraries[pi->LibraryIndex]);

		pi->Descriptor = NULL;

	// Decrement reference counter for library, and unload if last
		if (li->RefCount > 0) {
			li->RefCount--;
			if (li->RefCount == 0) {

			// Unload library
				dlclose(li->Handle);
				li->Handle = NULL;
			}
		}
	}
}

// ****************************************************************************
// **                      SSM Specific Functions                            **
// ****************************************************************************

unsigned long
LADSPAInfo::GetIDFromFilenameAndLabel(std::string filename,
                                      std::string label)
{
	bool library_loaded = false;

	if (m_FilenameLookup.find(filename) == m_FilenameLookup.end()) {
		cerr << "LADSPA Library " << filename << " not found!" << endl;
		return 0;
	}

	unsigned long library_index = m_FilenameLookup[filename];

	if (!(m_Libraries[library_index].Handle)) library_loaded = true;

	LADSPA_Descriptor_Function desc_func = GetDescriptorFunctionForLibrary(library_index);

	if (!desc_func) {
		return 0;
	}

// Search for label in library
	const LADSPA_Descriptor *desc;
	for (unsigned long i = 0; (desc = desc_func(i)) != NULL; i++) {
		string l = desc->Label;
		if (l == label) {

		// If we had to load the library, unload it
			unsigned long id = desc->UniqueID;
			if (library_loaded) {
				dlclose(m_Libraries[library_index].Handle);
				m_Libraries[library_index].Handle = NULL;
			}
			return id;
		}
	}

	cerr << "Plugin " << label << " not found in library " << filename << endl;
	return 0;
}

const vector<LADSPAInfo::PluginEntry>
LADSPAInfo::GetMenuList(void)
{
	m_SSMMenuList.clear();

	DescendGroup("", "LADSPA", 1);

	return m_SSMMenuList;
}

const vector<LADSPAInfo::PluginInfo>
LADSPAInfo::GetPluginInfo(void)
{
    return m_Plugins;
}

unsigned long
LADSPAInfo::GetPluginListEntryByID(unsigned long unique_id)
{
	unsigned long j = 0;
	for (vector<PluginEntry>::iterator i = m_SSMMenuList.begin();
		i != m_SSMMenuList.end(); i++, j++) {
		if (i->UniqueID == unique_id) return j;
	}
	return m_SSMMenuList.size();
}

// ****************************************************************************
// **                     Private Member Functions                           **
// ****************************************************************************

// Build a list of plugins by group, suitable for SSM LADSPA Plugin drop-down
// The top-level "LADSPA" group is not included

void
LADSPAInfo::DescendGroup(string prefix,
                         const string group,
                         unsigned int depth)
{
	list<string> groups = GetSubGroups(group);

	if (prefix.length() > 0) {
	// Add an explicit '/' as we're creating sub-menus from groups
		prefix += "/";
	}

	for (list<string>::iterator g = groups.begin(); g != groups.end(); g++) {
		string name;

		// Escape '/' and '|' characters
		size_t x = g->find_first_of("/|");
		if (x == string::npos) {
			name = *g;
		} else {
                    size_t last_x = 0;
			while (x < string::npos) {
				name += g->substr(last_x, x - last_x) + '\\' + (*g)[x];
				last_x = x + 1;
				x = g->find_first_of("/|", x + 1);
			}
			name += g->substr(last_x, x - last_x);
		}

		DescendGroup(prefix + name, *g, depth + 1);
	}
	if (m_RDFLabelLookup.find(group) != m_RDFLabelLookup.end()) {
		unsigned long uri_index = m_RDFLabelLookup[group];

	// Create group for unclassified plugins
		if (prefix.length() == 0) {
			prefix = "Unclassified/";
			depth = depth + 1;
		}

	// Temporary list (for sorting the plugins by name)
		list<PluginEntry> plugins;

		for (vector<unsigned long>::iterator p = m_RDFURIs[uri_index].Plugins.begin();
			p != m_RDFURIs[uri_index].Plugins.end(); p++) {

			PluginInfo *pi = &(m_Plugins[*p]);
			string name;

		// Escape '/' and '|' characters
			size_t x = pi->Name.find_first_of("/|");
			if (x == string::npos) {
				name = pi->Name;
			} else {
				size_t last_x = 0;
				while (x < string::npos) {
					name += pi->Name.substr(last_x, x - last_x) + '\\' + pi->Name[x];
					last_x = x + 1;
					x = pi->Name.find_first_of("/|", x + 1);
				}
				name += pi->Name.substr(last_x, x - last_x);
			}

			PluginEntry pe;

			pe.Depth = depth;
			pe.UniqueID = pi->UniqueID;
			pe.Name = prefix + name;
                  
			plugins.push_back(pe);
		}
		plugins.sort();

	// Deal with duplicates by numbering them
		for (list<PluginEntry>::iterator i = plugins.begin();
			i != plugins.end(); ) {
			string name = i->Name;

			i++;
			unsigned long n = 2;
			while ((i != plugins.end()) && (i->Name == name)) {
				stringstream s;
				s << n;
				i->Name = name + " (" + s.str() + ")";
				n++;
				i++;
			}
		}

	// Add all ordered entries to the Menu List
	// This ensures that plugins appear after groups
		for (list<PluginEntry>::iterator p = plugins.begin(); p != plugins.end(); p++) {
			m_SSMMenuList.push_back(*p);
		}
	}
}

// Get list of groups that are within given group. The root group is
// always "LADSPA"
list<string>
LADSPAInfo::GetSubGroups(const string group)
{
	list<string> groups;
	unsigned long uri_index;

	if (m_RDFLabelLookup.find(group) == m_RDFLabelLookup.end()) {
		return groups;
	} else {
		uri_index = m_RDFLabelLookup[group];
	}

	for (vector<unsigned long>::iterator sg = m_RDFURIs[uri_index].Children.begin();
		sg != m_RDFURIs[uri_index].Children.end(); sg++) {
		groups.push_back(m_RDFURIs[*sg].Label);
	}

	groups.sort();

	return groups;
}

// Unload any loaded DLLs and clear vectors etc
void
LADSPAInfo::CleanUp(void)
{
	m_MaxInputPortCount = 0;

	m_IDLookup.clear();
	m_Plugins.clear();

// Unload loaded dlls
	for (vector<LibraryInfo>::iterator i = m_Libraries.begin();
		i != m_Libraries.end(); i++) {
		if (i->Handle) dlclose(i->Handle);
	}

	m_Libraries.clear();
	m_Paths.clear();

	m_RDFURILookup.clear();
	m_RDFURIs.clear();

	if (m_ExtraPaths) {
		free(m_ExtraPaths);
		m_ExtraPaths = NULL;
	}
}

// Given a colon-separated list of paths, examine the contents of each
// path, examining any regular files using the given member function,
// which currently can be:
//
//   ExaminePluginLibrary - add plugin library info from plugins
//   ExamineRDFFile       - add plugin information from .rdf/.rdfs files
void
LADSPAInfo::ScanPathList(const char *path_list,
                         void (LADSPAInfo::*ExamineFunc)(const string,
                                                         const string))
{
	const char *start;
	const char *end;
	int extra;
	char *path;
	string basename;
	DIR *dp;
	struct dirent *ep;
	struct stat sb;

// This does the same kind of thing as strtok, but strtok won't
// like the const
	start = path_list;
	while (*start != '\0') {
		while (*start == ':') start++;
		end = start;
		while (*end != ':' && *end != '\0') end++;

		if (end - start > 0) {
			extra = (*(end - 1) == '/') ? 0 : 1;
			path = (char *)malloc(end - start + 1 + extra);
			if (path) {
				strncpy(path, start, end - start);
				if (extra == 1) path[end - start] = '/';
				path[end - start + extra] = '\0';

				dp = opendir(path);
				if (!dp) {
					cerr << "WARNING: Could not open path " << path << endl;
				} else {
					while ((ep = readdir(dp))) {

					// Stat file to get type
						basename = ep->d_name;
						if (!stat((path + basename).c_str(), &sb)) {

						// We only want regular files
							if (S_ISREG(sb.st_mode)) (*this.*ExamineFunc)(path, basename);
						}
					}
					closedir(dp);
				}
				free(path);
			}
		}
		start = end;
	}
}

// Check given file is a valid LADSPA Plugin library
//
// If so, add path, library and plugin info
// to the m_Paths, m_Libraries and m_Plugins vectors.
//
void
LADSPAInfo::ExaminePluginLibrary(const string path,
                                 const string basename)
{
	void *handle;
	LADSPA_Descriptor_Function desc_func;
	const LADSPA_Descriptor *desc;
	string fullpath = path + basename;

// We're not executing any code, so be lazy about resolving symbols
	handle = dlopen(fullpath.c_str(), RTLD_LAZY);

	if (!handle) {
		cerr << "WARNING: File " << fullpath
			<< " could not be examined" << endl;
		cerr << "dlerror() output:" << endl;
		cerr << dlerror() << endl;
	} else {

	// It's a DLL, so now see if it's a LADSPA plugin library
		desc_func = (LADSPA_Descriptor_Function)dlsym(handle,
													"ladspa_descriptor");
		if (!desc_func) {

		// Is DLL, but not a LADSPA one
			cerr << "WARNING: DLL " << fullpath
				<< " has no ladspa_descriptor function" << endl;
			cerr << "dlerror() output:" << endl;
			cerr << dlerror() << endl;
		} else {

		// Got ladspa_descriptor, so we can now get plugin info
			bool library_added = false;
			unsigned long i = 0;
			desc = desc_func(i);
			while (desc) {

			// First, check that it's not a dupe
				if (m_IDLookup.find(desc->UniqueID) != m_IDLookup.end()) {
					unsigned long plugin_index = m_IDLookup[desc->UniqueID];
					unsigned long library_index = m_Plugins[plugin_index].LibraryIndex;
					unsigned long path_index = m_Libraries[library_index].PathIndex;

					cerr << "WARNING: Duplicated Plugin ID ("
						<< desc->UniqueID << ") found:" << endl;

					cerr << "  Plugin " << m_Plugins[plugin_index].Index
						<< " in library: " << m_Paths[path_index]
						<< m_Libraries[library_index].Basename
						<< " [First instance found]" << endl;
					cerr << "  Plugin " << i << " in library: " << fullpath
						<< " [Duplicate not added]" << endl;
				} else {
					if (CheckPlugin(desc)) {

					// Add path if not already added
						unsigned long path_index;
						vector<string>::iterator p = find(m_Paths.begin(), m_Paths.end(), path);
						if (p == m_Paths.end()) {
							path_index = m_Paths.size();
							m_Paths.push_back(path);
						} else {
							path_index = p - m_Paths.begin();
						}

					// Add library info if not already added
						if (!library_added) {
							LibraryInfo li;
							li.PathIndex = path_index;
							li.Basename = basename;
							li.RefCount = 0;
							li.Handle = NULL;
							m_Libraries.push_back(li);

							library_added = true;
						}

                                                if ( ! desc->Name )
                                                {
                                                    printf( "WARNING: LADSPA Plugin with id %lu has no name!\n", desc->UniqueID );

                                                    continue;
                                                }

					// Add plugin info
						PluginInfo pi;
						pi.LibraryIndex = m_Libraries.size() - 1;
						pi.Index = i;
						pi.UniqueID = desc->UniqueID;
						pi.Label = desc->Label;
						pi.Name = desc->Name;
						pi.Descriptor = NULL;
                                                pi.Maker = desc->Maker;
                                                pi.AudioInputs = 0;
                                                pi.AudioOutputs = 0;
                                                
					// Find number of input ports
						unsigned long in_port_count = 0;
						for (unsigned long p = 0; p < desc->PortCount; p++) {
                                                    if (LADSPA_IS_PORT_INPUT(desc->PortDescriptors[p])) {
                                                        in_port_count++;
                                                        if ( LADSPA_IS_PORT_AUDIO(desc->PortDescriptors[p] ) )
                                                            pi.AudioInputs++;
                                                    }
						}
                                                for (unsigned long p = 0; p < desc->PortCount; p++) {
                                                    if (LADSPA_IS_PORT_OUTPUT(desc->PortDescriptors[p])) {
                                                        
                                                        if ( LADSPA_IS_PORT_AUDIO(desc->PortDescriptors[p] ) )
                                                            pi.AudioOutputs++;
                                                    }
						}
					
						if (in_port_count > m_MaxInputPortCount) {
                                                    m_MaxInputPortCount = in_port_count;
                                                    
						}

						m_Plugins.push_back(pi);

					// Add to index
						m_IDLookup[desc->UniqueID] = m_Plugins.size() - 1;

					} else {
						cerr << "WARNING: Plugin " << desc->UniqueID << " not added" << endl;
					}
				}

				desc = desc_func(++i);
			}
		}
		dlclose(handle);
	}
}

#ifdef HAVE_LIBLRDF
// Examine given RDF plugin meta-data file
void
LADSPAInfo::ExamineRDFFile(const std::string path,
                           const std::string basename)
{
	string fileuri = "file://" + path + basename;

	if (lrdf_read_file(fileuri.c_str())) {
		cerr << "WARNING: File " << path + basename << " could not be parsed [Ignored]" << endl;
	}
}

// Recursively add rdf information for plugins that have been
// found from scanning LADSPA_PATH
void
LADSPAInfo::MetadataRDFDescend(const char * uri,
                               unsigned long parent)
{
	unsigned long this_uri_index;

// Check URI not already added
	if (m_RDFURILookup.find(uri) == m_RDFURILookup.end()) {

	// Not found
		RDFURIInfo ri;

		ri.URI = uri;

		if (ri.URI == LADSPA_BASE "Plugin") {

		// Add top level group as "LADSPA"
		// This will always happen, even if there are no .rdf files read by liblrdf
		// or if there is no liblrdf support
			ri.Label = "LADSPA";
		} else {
			char * label = lrdf_get_label(uri);
			if (label) {
				ri.Label = label;
			} else {
				ri.Label = "(No label)";
			}
		}

	// Add any instances found
		lrdf_uris * instances = lrdf_get_instances(uri);
		if (instances) {
			for (unsigned long j = 0; j < instances->count; j++) {
				unsigned long uid = lrdf_get_uid(instances->items[j]);
				if (m_IDLookup.find(uid) != m_IDLookup.end()) {
					ri.Plugins.push_back(m_IDLookup[uid]);
				}
			}
		}

		lrdf_free_uris(instances);

		m_RDFURIs.push_back(ri);
		this_uri_index = m_RDFURIs.size() - 1;

		m_RDFURILookup[ri.URI] = this_uri_index;
		m_RDFLabelLookup[ri.Label] = this_uri_index;

	} else {

	// Already added
		this_uri_index = m_RDFURILookup[uri];
	}

// Only add parent - child info if this uri is NOT the first (root) uri
	if (this_uri_index > 0) {
		m_RDFURIs[this_uri_index].Parents.push_back(parent);
		m_RDFURIs[parent].Children.push_back(this_uri_index);
	}

	lrdf_uris * uris = lrdf_get_subclasses(uri);

	if (uris) {
		for (unsigned long i = 0; i < uris->count; i++) {
			MetadataRDFDescend(uris->items[i], this_uri_index);
		}
	}

	lrdf_free_uris(uris);
}
#endif

bool
LADSPAInfo::CheckPlugin(const LADSPA_Descriptor *desc)
{
#define test(t, m) { \
	if (!(t)) { \
		cerr << m << endl; \
		return false; \
	} \
}
	test(desc->instantiate, "WARNING: Plugin has no instatiate function");
	test(desc->connect_port, "WARNING: Warning: Plugin has no connect_port funciton");
	test(desc->run, "WARNING: Plugin has no run function");
	test(!(desc->run_adding != 0 && desc->set_run_adding_gain == 0),
			"WARNING: Plugin has run_adding but no set_run_adding_gain");
	test(!(desc->run_adding == 0 && desc->set_run_adding_gain != 0),
			"WARNING: Plugin has set_run_adding_gain but no run_adding");
	test(desc->cleanup, "WARNING: Plugin has no cleanup function");
	test(!LADSPA_IS_INPLACE_BROKEN(desc->Properties),
			"WARNING: Plugin cannot use in place processing");
	test(desc->PortCount, "WARNING: Plugin has no ports");

	return true;
}

LADSPA_Descriptor_Function
LADSPAInfo::GetDescriptorFunctionForLibrary(unsigned long library_index)
{
	LibraryInfo *li = &(m_Libraries[library_index]);

	if (!(li->Handle)) {

	// Need full path
		string fullpath = m_Paths[li->PathIndex];
		fullpath.append(li->Basename);

	// Immediate symbol resolution, as plugin code is likely to be executed
		li->Handle = dlopen(fullpath.c_str(), RTLD_NOW);
		if (!(li->Handle)) {

		// Plugin library changed since last path scan
			cerr << "WARNING: Plugin library " << fullpath << " cannot be loaded" << endl;
			cerr << "Rescan of plugins recommended" << endl;
			cerr << "dlerror() output:" << endl;
			cerr << dlerror() << endl;
			return NULL;
		}
	}

// Got handle so now verify that it's a LADSPA plugin library
	const LADSPA_Descriptor_Function desc_func = (LADSPA_Descriptor_Function)dlsym(li->Handle,
																				"ladspa_descriptor");
	if (!desc_func) {

	// Is DLL, but not a LADSPA one (changed since last path scan?)
		cerr << "WARNING: DLL " << m_Paths[li->PathIndex] << li->Basename
			<< " has no ladspa_descriptor function" << endl;
		cerr << "Rescan of plugins recommended" << endl;
		cerr << "dlerror() output:" << endl;
		cerr << dlerror() << endl;

	// Unload library
		dlclose(li->Handle);
		return NULL;
	}

	return desc_func;
}
