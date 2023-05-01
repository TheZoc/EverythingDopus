/**
 * EverythingDopus (c) 2023 Felipe Guedes da Silveira
 *
 * This is a script for Directory Opus.
 * See https://www.gpsoft.com.au/DScripts/redirect.asp?page=scripts for development information.
 *
 * This is the script interface between Directory Opus and EverythingDopusCLI
 *
 * Acknowledgements:
 * This script uses getResourcePathFromGlobalVar() from ExternalCompare.js by
 * Wowbagger. It is used to simplify the setup process, allowing the user to
 * easily choose the location of EverythingDopusCLI ed.exe executable.
 */


// Called by Directory Opus to initialize the script
function OnInit(initData)
{
	initData.name = "EverythingDopus";
	initData.version = "2.0 beta1";
	initData.copyright = "(c) 2023 Felipe Guedes da Silveira";
	initData.url = "https://github.com/TheZoc/EverythingDopus";
	initData.desc = "";
	initData.default_enable = true;
	initData.min_version = "12.0";

	// variable for enabling debug, so as to enable debug via a button
	initData.config_desc = DOpus.NewMap();
	initData.config.DEBUG = false;
	initData.config_desc("DEBUG") = "Set DEBUG flag to true in order to enable logging messages to the Opus Output Window";

	initData.config.DebugEnableVar = "$glob:debug";
	initData.config_desc("DebugEnableVar") = "Global var for debug override. Used to enable and disable debug for script applications.";

	var searchCmd = initData.AddCommand();
	searchCmd.name     = "EverythingDopus";
	searchCmd.method   = "OnEverythingDopusSearch";
	searchCmd.desc     = "Search given string on Everything and add results to a Directory Opus collection";
	searchCmd.label    = "Everything Dopus Search";
	searchCmd.template = "SearchString/R";

	var searchDlgCmd = initData.AddCommand();
	searchDlgCmd.name     = "EverythingDopusDialog";
	searchDlgCmd.method   = "OnEverythingDopusDialog";
	searchDlgCmd.desc     = "Display the search dialog for EverythingDopus";
	searchDlgCmd.label    = "Everything Dopus Dialog";
	searchDlgCmd.template = "";
}

// Implement the EverythingDopus command
function OnEverythingDopusSearch(scriptCmdData)
{
	var exePath = RetrieveEverythingDopusPath(scriptCmdData);
	if (exePath == null)
	{
		DOpus.OutputString(scriptCmdData.func,"OnEverythingDopusSearch() - The executable 'ed.exe' for application EverythingDopus could not be found. Try running the script again.");
		return;
	}

	// Extract the search string
	LogMessage("OnEverythingDopusSearch() - SearchString: " + scriptCmdData.func.args.SearchString);
	var searchString = scriptCmdData.func.args.got_arg.SearchString ? scriptCmdData.func.args.SearchString : "";

	// Build the command line
	var commandLine = exePath + " " + searchString;
	LogMessage("OnEverythingDopusSearch() - Command line: " + commandLine);
	scriptCmdData.func.command.runcommand(commandLine);
}

// Implement the EverythingDopusDialog command
function OnEverythingDopusDialog(scriptCmdData)
{
	DumpSearchHistory();
	var exePath = RetrieveEverythingDopusPath(scriptCmdData);
	if (exePath == null)
	{
		DOpus.OutputString(scriptCmdData.func,"OnEverythingDopusDialog() - The executable 'ed.exe' for application EverythingDopus could not be found. Try running the script again.");
		return;
	}

	// Create the user dialog
	var dlg = DOpus.Dlg;
	dlg.title = "EverythingDopus";
	dlg.template = "dlgEverythingDopusSearchDialog";
	dlg.detach = true;
	dlg.Create();
	dlg.Control("txtTitle").style = "b";
	dlg.Control("txtIcon").label = Script.LoadImage("EverythingDopusCLI-32x32.png");

	// Populate the search combo box
	for (var i = 0; i < 10; ++i)
	{
		var currentItem = "EverythingDopusHistory" + i.toString();
		if (!DOpus.vars.Exists(currentItem))
		{
			break;
		}
		dlg.Control("cmbSearch").AddItem(DOpus.Vars.Get(currentItem));
	}

	// Select the first item in the combobox, if available
	if (dlg.Control("cmbSearch").count > 0)
	{
		dlg.Control("cmbSearch").SelectItem(0)
	}

	// Dialog message loop
	for (var msg = dlg.GetMsg(); msg.result; msg = dlg.GetMsg())
	{
		// Event handler snippet
		// DOpus.Output("Msg Event = " + msg.event);
	}

	var searchString = ""
	switch(dlg.result)
	{
	case 1: // Ok
		LogMessage("Search Pressed");
		searchString = dlg.Control("cmbSearch").label;
		break;
	case 2: // Search local folder only
		searchString = "\"" + scriptCmdData.func.sourcetab.path + "\" " + dlg.Control("cmbSearch").label;
		LogMessage("Search Folder Pressed - searchString: " + searchString);
		break;
	default: // Everything else
		LogMessage("Return code = " + dlg.result);
		return;
	};

	LogMessage("OnEverythingDopusDialog() - searchString: " + searchString);
	AddSearchToHistory(searchString);

	var commandLine = exePath + " " + searchString;
	LogMessage("OnEverythingDopusDialog() - Command line: " + commandLine);
	scriptCmdData.func.command.runcommand(commandLine);
}

/**
 * Utility Functions
 */

// Retrieves the path for ex.exe, stored in dopus variables
function RetrieveEverythingDopusPath(scriptCmdData)
{
	// Check if the user is holding shift when they clicked the button
	var qualifiers = scriptCmdData.func.qualifiers;
	var fromKeyboard = scriptCmdData.func.fromkey;
	var forceRequestForExternalTool = (!fromKeyboard && (qualifiers.indexOf("shift") > -1));
	LogMessage("RetrieveEverythingDopusPath() - fromKeyboard: " + fromKeyboard + " - forceRequestForExternalTool:" + forceRequestForExternalTool);

	// Get the exe path for the command line search application
	var doFsu = DOpus.FSUtil;
	var exePath = getResourcePathFromGlobalVar("EverythingDopusCLI", "ed.exe", "EverythingDopusCLI", forceRequestForExternalTool);
	if (!exePath || !doFsu.Exists(exePath))
	{
		DOpus.OutputString(scriptCmdData.func,"RetrieveEverythingDopusPath() - The executable 'ed.exe' for application EverythingDopus could not be found. Try running the script again.");
		return null;
	}

	return exePath;
}

// Add an entry to the search history, stored in dopus variables
function AddSearchToHistory(searchString)
{
	LogMessage("AddSearchToHistory() - searchString: " + searchString);

	// Keep the last 10 search entries
	var doVars = DOpus.vars;
	var foundIndex = -1;
	var lastValidIndex = -1;
	for (var i = 0; i < 10; ++i)
	{
		// Check if we're at the end of the existing history
		var currentItem = "EverythingDopusHistory" + i.toString();
		lastValidIndex = i;
		if (!doVars.Exists(currentItem))
		{
			LogMessage("AddSearchToHistory() - last valid history index: " + lastValidIndex);
			break;
		}
		else if (doVars.Get(currentItem) == searchString)
		{
			LogMessage("AddSearchToHistory() - doVars.Get(\"" + currentItem + "\"): " + doVars.Get(currentItem));
			foundIndex = i;
			break;
		}
	}

  // If we found it, put it on the top of the stack
	if (foundIndex != -1)
	{
		LogMessage("AddSearchToHistory() - searchString found in index " + i + " - Moving to the top of the history");
		ShiftSearchHistoryDown(foundIndex);
	}

	// If we have valid indices, move everything down in the stack by 1, overwriting the last entry
	else if (lastValidIndex > -1)
	{
		LogMessage("AddSearchToHistory() - Last valid index is " + lastValidIndex + ", shifting history down.");
		ShiftSearchHistoryDown(lastValidIndex);
	}

	LogMessage("AddSearchToHistory() - Setting EverythingDopusHistory0 to the search string");
	doVars.Set("EverythingDopusHistory0", searchString);
}

// This function shifts everything in the search history down.
// It starts at the given index and move everything down until the top of the stack.
function ShiftSearchHistoryDown(startIndex)
{
	for (var i = startIndex; i > 0; --i)
	{
		DOpus.vars.Set("EverythingDopusHistory" + i.toString(), DOpus.vars.Get("EverythingDopusHistory" + (i-1).toString()));
	}
}

// Outputs all search history variables and their data to the console
function DumpSearchHistory()
{
	if (!Script.config.DEBUG && !doLogCmd.IsSet(Script.config.DebugEnableVar))
		return;

	for (var i = 0; i < 10; ++i)
	{
		var currentItem = "EverythingDopusHistory" + i.toString();
		if (!DOpus.vars.Exists(currentItem))
		{
			DOpus.OutputString(currentItem + ": does not exist yet.");
		}
		else
		{
			DOpus.OutputString(currentItem + ": " + DOpus.Vars.Get(currentItem));
		}
	}
}

var doLogCmd = DOpus.NewCommand;
function LogMessage(message)
{
  if (Script.config.DEBUG || doLogCmd.IsSet(Script.config.DebugEnableVar)) { DOpus.OutputString(message) };
}

/*
Requested from the user via a file dialog, the path to an executable. The path is then stored in a dopus persistent variable.
Making it accessible next run. Caching pattern.
v1.0

name: name of the application, used in the dialog to assist user.
filename: name of requested file. use in dialog.
varKey: dopus variable key, where the exe path is stored.
forceUserRequest: Force user to find path even if the path exists in the variable. Useful if the user needs to reset. Pass if true shift is down.
*/
function getResourcePathFromGlobalVar(name, filename, varKey, forceUserRequest)
{
	var doFsu = DOpus.FSUtil;
	var doCmd = DOpus.NewCommand;
	var doVars = DOpus.vars;

	var requestReason;

	var resourcePath;

	if(forceUserRequest)
	{
		requestReason = "forced";
	}
	else if(!doVars.Exists(varKey))
	{
		requestReason = "key'" + varKey + "' not found";
	}
	else
	{
		resourcePath = doFsu.Resolve(doVars.Get(varKey));
		if (!doFsu.Exists(resourcePath))
		{
			requestReason = "Path not found:" + resourcePath;
		}
	}

	if(!resourcePath || !doFsu.Exists(resourcePath))
	{
		var dlg = DOpus.Dlg;
		var dlgResult = dlg.Open("Please locate the executable '" +filename + "' for application " + name + " ["+requestReason+"]", doFsu.Resolve("/homeroot"), DOpus.Listers(0));
		if(dlgResult && dlgResult.result)
		{
			resourcePath = doFsu.Resolve(dlgResult);
			if(resourcePath || doFsu.Exists(resourcePath))
			{
				doVars(varKey) = resourcePath;
				doVars(varKey).persist = true;
			}
			else
			{
				var dlg = DOpus.Dlg;
				dlg.title = name + " not found";
				dlg.message = "Can't find file\n Path:" + resourcePath ;
				dlg.buttons = "OK";
				dlg.icon = "error";
				dlg.Show();
				doVars(varKey).Delete();
				resourcePath = null;
			}
		}
		else
		{
			doVars(varKey).Delete();
			resourcePath = null;
		}
	}

	LogMessage("OnEverythingDopusSearch() - Path for " + name + " [key: " + varKey + "] = " + resourcePath);

	return resourcePath;
}

==SCRIPT RESOURCES
<resources>
	<resource name="dlgEverythingDopusSearchDialog" type="dialog">
		<dialog fontsize="8" height="72" lang="english" title="Directory Opus" width="318">
			<control halign="left" height="8" name="txtTitle" title="Enter Everything Search Query" type="static" valign="top" width="276" x="32" y="6" />
			<control halign="left" height="8" name="txtBody" title="You can do a /regex \d\d\d/ search using slashes" type="static" valign="top" width="276" x="32" y="18" />
			<control halign="center" height="22" image="yes" name="txtIcon" title="Icon" type="static" valign="top" width="22" x="6" y="6" />
			<control edit="yes" height="14" name="cmbSearch" type="combo" width="306" x="6" y="36" />
			<control close="0" height="14" name="btnCancel" title="&Cancel" type="button" width="62" x="252" y="54" />
			<control close="1" default="yes" height="14" name="btnOK" title="&Search" type="button" width="62" x="186" y="54" />
			<control close="2" height="14" name="btnLocalSearch" title="Search &Folder" type="button" width="62" x="120" y="54" />
		</dialog>
	</resource>
</resources>
