//
//  LADSPAInfo.h - Header file for LADSPA Plugin info class
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

#ifndef __ladspa_info_h__
#define __ladspa_info_h__

// #include <config.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <ladspa.h>

class LADSPAInfo
{
public:
// If override is false, examine $LADSPA_PATH
// Also examine supplied path list
// For all paths, add basic plugin information for later lookup,
// instantiation and so on.
	LADSPAInfo(bool override = false, const char *path_list = "");

// Unload all loaded plugins and clean up
	~LADSPAInfo();

// ************************************************************************
// Loading/Unloading plugin libraries
//
// At first, no library dlls are loaded.
//
// A plugin library may have more than one plugin descriptor. The
// descriptor is used to instantiate, activate, execute plugin instances.
// Administration of plugin instances are outwith the scope of this class,
// instead, descriptors are requested using GetDecriptorByID, and disposed
// of using DiscardDescriptorByID.
//
// Each library keeps a reference count of descriptors requested. A library
// is loaded when a descriptor is requested for the first time, and remains
// loaded until the number of discards matches the number of requests.

// Rescan all paths in $LADSPA_PATH, as per constructor.
// This will also unload all libraries, and make any descriptors that
// have not been discarded with DiscardDescriptorByID invalid.
	void                            RescanPlugins(void);

// Unload all dlopened libraries. This will make any descriptors that
// have not been discarded with DiscardDescriptorByID invalid.
	void                            UnloadAllLibraries(void);

// Get descriptor of plugin with given ID. This increments the descriptor
// count for the corresponding library.
	const LADSPA_Descriptor        *GetDescriptorByID(unsigned long unique_id);

// Notify that a descriptor corresponding to the given ID has been
// discarded. This decrements the descriptor count for the corresponding
// library.
	void                            DiscardDescriptorByID(unsigned long unique_id);

// ************************************************************************
// SSM Specific options

// Get unique ID of plugin identified by given library filename and label.
// This is for backwards compatibility with older versions of SSM where the
// path and label of the plugin was stored in the configuration - current
// versions store the Unique ID
	unsigned long                   GetIDFromFilenameAndLabel(std::string filename,
	                                                          std::string label);

// Struct for plugin information returned by queries
	struct PluginEntry
	{
		unsigned int    Depth;
		unsigned long   UniqueID;
		std::string     Name;
            std::string Category;

		bool operator<(const PluginEntry& pe)
		{
			return (Name<pe.Name);
		}
	};


// For cached plugin information
	struct PluginInfo
	{
		unsigned long               LibraryIndex;   // Index of library in m_Libraries
		unsigned long               Index;          // Plugin index in library
		unsigned long               UniqueID;       // Unique ID
		std::string                 Label;          // Plugin label
		std::string                 Name;           // Plugin Name
            std::string Maker;
                  unsigned int AudioInputs;
            unsigned int AudioOutputs;
            const LADSPA_Descriptor    *Descriptor;     // Descriptor, NULL

            
	};

// Get ordered list of plugin names and IDs for plugin menu
	const std::vector<PluginEntry>  GetMenuList(void);
        
        const std::vector<PluginInfo> GetPluginInfo(void);

// Get the index in the above list for given Unique ID
// If not found, this returns the size of the above list
	unsigned long                   GetPluginListEntryByID(unsigned long unique_id);

// Get the number of input ports for the plugin with the most
// input ports
	unsigned long                   GetMaxInputPortCount(void) { return m_MaxInputPortCount; }

private:
// See LADSPAInfo.C for comments on these functions
	void                            DescendGroup(std::string prefix,
	                                             const std::string group,
	                                             unsigned int depth);
	std::list<std::string>          GetSubGroups(const std::string group);

	void                            CleanUp(void);
	void                            ScanPathList(const char *path_list,
	                                             void (LADSPAInfo::*ExamineFunc)(const std::string,
	                                                                             const std::string));
	void                            ExaminePluginLibrary(const std::string path,
	                                                     const std::string basename);

	bool                            CheckPlugin(const LADSPA_Descriptor *desc);
	LADSPA_Descriptor_Function      GetDescriptorFunctionForLibrary(unsigned long library_index);
#ifdef HAVE_LIBLRDF
	void                            ExamineRDFFile(const std::string path,
	                                               const std::string basename);
	void                            MetadataRDFDescend(const char *uri,
	                                                   unsigned long parent);
#endif

// For cached library information
	struct LibraryInfo
	{
		unsigned long               PathIndex;      // Index of path in m_Paths
		std::string                 Basename;       // Filename
		unsigned long               RefCount;       // Count of descriptors requested
		void                       *Handle;         // DLL Handle, NULL
	};

// For cached RDF uri information
	struct RDFURIInfo
	{
		std::string                 URI;            // Full URI for use with lrdf
		std::string                 Label;          // Label
		std::vector<unsigned long>  Parents;        // Index of parents in m_RDFURIs
		std::vector<unsigned long>  Children;       // Indices of children in m_RDFURIs
		std::vector<unsigned long>  Plugins;        // Indices of plugins in m_Plugins
	};

// Lookup maps
	typedef std::map<unsigned long,
	                 unsigned long,
	                 std::less<unsigned long> >  IDMap;

	typedef std::map<std::string,
	                 unsigned long,
	                 std::less<std::string> >    StringMap;

	bool                            m_LADSPAPathOverride;
	char                           *m_ExtraPaths;

// LADSPA Plugin information database
	std::vector<std::string>        m_Paths;
	std::vector<LibraryInfo>        m_Libraries;
	std::vector<PluginInfo>         m_Plugins;

// Plugin lookup maps
	IDMap                           m_IDLookup;

// RDF URI database
	std::vector<RDFURIInfo>         m_RDFURIs;

// RDF URI lookup map
	StringMap                       m_RDFURILookup;

// RDF Label lookup map
	StringMap                       m_RDFLabelLookup;

// SSM specific data
	std::vector<PluginEntry>        m_SSMMenuList;
	StringMap                       m_FilenameLookup;
	unsigned long                   m_MaxInputPortCount;
};

#endif // __ladspa_info_h__
