use zed_extension_api::{self as zed, settings::LspSettings};

struct LumaExtension;

impl zed::Extension for LumaExtension {
    fn new() -> Self {
        Self
    }

    fn language_server_command(
        &mut self,
        language_server_id: &zed::LanguageServerId,
        worktree: &zed::Worktree,
    ) -> zed::Result<zed::Command> {
        if language_server_id.as_ref() != "luma" {
            return Err(format!("Unknown language server ID {language_server_id}"));
        }

        let binary_settings = LspSettings::for_worktree("luma", worktree)?.binary;
        let args = binary_settings
            .as_ref()
            .and_then(|settings| settings.arguments.clone())
            .unwrap_or_else(|| vec!["--lsp".into()]);
        let command = binary_settings
            .and_then(|settings| settings.path)
            .or_else(|| worktree.which("luma"))
            .ok_or_else(|| {
                "luma was not found on PATH; configure lsp.luma.binary.path".to_string()
            })?;

        Ok(zed::Command {
            command,
            args,
            env: Default::default(),
        })
    }
}

zed::register_extension!(LumaExtension);
