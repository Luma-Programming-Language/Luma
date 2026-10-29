const fs = require("fs");
const path = require("path");
const vscode = require("vscode");
const {
  CloseAction,
  ErrorAction,
  LanguageClient,
  TransportKind
} = require("vscode-languageclient/node");

const DEFAULT_SERVER_COMMAND = "luma";
const MAX_RESTARTS = 5;

let client;
let serverCommand;
let outputChannel;

function isExecutableFile(filePath) {
  try {
    if (!fs.statSync(filePath).isFile()) {
      return false;
    }
    fs.accessSync(filePath, process.platform === "win32" ? fs.constants.F_OK : fs.constants.X_OK);
    return true;
  } catch {
    return false;
  }
}

function resolveServerCommand() {
  const configuredPath = vscode.workspace
    .getConfiguration("luma")
    .get("serverPath", DEFAULT_SERVER_COMMAND)
    .trim();

  if (!configuredPath) {
    throw new Error('The "luma.serverPath" setting cannot be empty.');
  }

  if (configuredPath !== DEFAULT_SERVER_COMMAND) {
    if (path.isAbsolute(configuredPath) || configuredPath.includes("/") || configuredPath.includes("\\")) {
      const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath || process.cwd();
      const resolvedPath = path.resolve(workspaceRoot, configuredPath);
      if (!isExecutableFile(resolvedPath)) {
        throw new Error(`Luma executable was not found or is not executable: ${resolvedPath}`);
      }
      return resolvedPath;
    }
    return configuredPath;
  }

  const executableNames = process.platform === "win32"
    ? ["luma.exe"]
    : ["luma"];
  const workspaceFolders = vscode.workspace.workspaceFolders || [];

  for (const folder of workspaceFolders) {
    for (const executableName of executableNames) {
      const candidates = [
        path.join(folder.uri.fsPath, "bin", executableName),
        path.join(folder.uri.fsPath, executableName)
      ];
      for (const candidate of candidates) {
        if (isExecutableFile(candidate)) {
          return candidate;
        }
      }
    }
  }

  return DEFAULT_SERVER_COMMAND;
}

function createErrorHandler() {
  let restartCount = 0;

  return {
    error(error, _message, count) {
      const message = error instanceof Error ? error.message : String(error);
      outputChannel.appendLine(`[LSP] Protocol error (${count}): ${message}`);
      if (count >= 20) {
        outputChannel.appendLine("[LSP] Too many protocol errors; stopping the server.");
        outputChannel.show(true);
        return { action: ErrorAction.Shutdown };
      }
      return { action: ErrorAction.Continue };
    },

    closed() {
      restartCount += 1;
      if (restartCount > MAX_RESTARTS) {
        outputChannel.appendLine(`[LSP] Server closed too many times; automatic restart stopped.`);
        outputChannel.show(true);
        return { action: CloseAction.DoNotRestart };
      }
      outputChannel.appendLine(`[LSP] Server connection closed; restarting (${restartCount}/${MAX_RESTARTS}).`);
      return { action: CloseAction.Restart };
    }
  };
}

function activate(context) {
  outputChannel = vscode.window.createOutputChannel("Luma Language Server");
  const traceOutputChannel = vscode.window.createOutputChannel("Luma Language Server Trace");
  context.subscriptions.push(outputChannel, traceOutputChannel);

  context.subscriptions.push(
    vscode.commands.registerCommand("luma.showServerPath", () => {
      vscode.window.showInformationMessage(`Luma executable: ${serverCommand || "not started"}`);
    }),
    vscode.commands.registerCommand("luma.showOutput", () => {
      outputChannel.show(true);
    }),
    vscode.commands.registerCommand("luma.restartServer", async () => {
      if (!client) {
        vscode.window.showWarningMessage("The Luma language server has not started.");
        return;
      }
      try {
        await client.restart();
        outputChannel.appendLine("[LSP] Server restarted.");
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        outputChannel.appendLine(`[LSP] Restart failed: ${message}`);
        outputChannel.show(true);
        vscode.window.showErrorMessage(`Luma language server restart failed: ${message}`);
      }
    })
  );

  try {
    serverCommand = resolveServerCommand();
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    outputChannel.appendLine(`[LSP] ${message}`);
    outputChannel.show(true);
    vscode.window.showErrorMessage(`${message} Set "luma.serverPath" in VS Code settings.`);
    return;
  }

  outputChannel.appendLine(`[LSP] Starting ${serverCommand} --lsp`);

  const serverOptions = {
    command: serverCommand,
    args: ["--lsp"],
    transport: TransportKind.stdio
  };
  const clientOptions = {
    documentSelector: [{ scheme: "file", language: "luma" }],
    synchronize: {
      fileEvents: vscode.workspace.createFileSystemWatcher("**/*.lx")
    },
    outputChannel,
    traceOutputChannel,
    errorHandler: createErrorHandler()
  };

  client = new LanguageClient("lumaLanguageServer", "Luma Language Server", serverOptions, clientOptions);
  context.subscriptions.push(client);

  return client.start().then(
    () => outputChannel.appendLine("[LSP] Client started."),
    (error) => {
      const message = error instanceof Error ? error.message : String(error);
      outputChannel.appendLine(`[LSP] Failed to start: ${message}`);
      outputChannel.show(true);
      vscode.window.showErrorMessage(`Luma language server failed to start: ${message}`);
    }
  );
}

function deactivate() {
  return client ? client.stop() : undefined;
}

module.exports = {
  activate,
  deactivate
};
