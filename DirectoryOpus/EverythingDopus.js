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
	initData.version = "1.0";
	initData.copyright = "(c) 2023 Felipe Guedes da Silveira";
	initData.url = "https://github.com/TheZoc/EverythingDopusCLI";
	initData.desc = "";
	initData.default_enable = true;
	initData.min_version = "12.0";

	// variable for enabling debug, so as to enable debug via a button
	initData.config_desc = DOpus.NewMap();
	initData.config.DEBUG = false;
	initData.config_desc("DEBUG") = "Set DEBUG flag to true in order to enable logging messages to the Opus Output Window";

	initData.config.DebugEnableVar = "$glob:debug";
	initData.config_desc("DebugEnableVar") = "Global var for debug override. Used to enable and disable debug for script applications.";

	var cmd = initData.AddCommand();
	cmd.name = "EverythingDopus";
	cmd.method = "OnEverythingDopusSearch";
	cmd.desc = "Search given string on Everything and add results to a Directory Opus collection";
	cmd.label = "Everything Dopus Search";
	cmd.template = "SearchString/R";
}


// Implement the ExternalCompare command
function OnEverythingDopusSearch(scriptCmdData)
{
	var funcData = scriptCmdData.func;
	LogMessage("OnEverythingDopusSearch - SearchString: " + funcData.args.SearchString);
	var searchString = funcData.args.got_arg.SearchString ? funcData.args.SearchString : "";
	var qualifiers = funcData.qualifiers;
	var fromKeyboard = funcData.fromkey;
	var forceRequestForExternalTool = (!fromKeyboard && (qualifiers.indexOf("shift") > -1));
	LogMessage("OnEverythingDopusSearch - fromKeyboard: " + fromKeyboard + " - forceRequestForExternalTool:" + forceRequestForExternalTool);

	//Get the exe path for the command line search application
	var doFsu = DOpus.FSUtil;
	var exePath = getResourcePathFromGlobalVar("EverythingDopusCLI", "ed.exe", "EverythingDopusCLI", forceRequestForExternalTool);
	if (!exePath || !doFsu.Exists(exePath))
	{
		RaiseError(funcData,"The executable 'ed.exe' for application EverythingDopusCLI could not be found. Try running the script again.");
		return;
	}

	var commandLine = exePath + " " + searchString;
	LogMessage("OnEverythingDopusSearch - command line:" + commandLine);
	funcData.command.runcommand(commandLine);
}

var doLogCmd = DOpus.NewCommand;
function LogMessage(message)
{
  if (Script.config.DEBUG || doLogCmd.IsSet(Script.config.DebugEnableVar)) { DOpus.OutputString(message)};
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

	LogMessage("OnEverythingDopusSearch - Path for " +name + " [key: " + varKey + "] = " + resourcePath);

	return resourcePath;
}