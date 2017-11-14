// SPDX-License-Identifier: GPL-2.0
/**
 * debugfs interface for nfs
 *
 * (c) 2017 Garmin International
 */

#include <linux/debugfs.h>
#include <linux/sunrpc/clnt.h>
#include <linux/nfs_fs.h>

#include "nfs4_fs.h"
#include "internal.h"

static struct dentry *topdir;
static struct dentry *nfs_server_dir;
static struct dentry *nfs_client_dir;

static struct dentry*
link_rpc_client(char const *name, struct rpc_clnt *client,
		struct dentry *parent)
{
	int len;
	char target[34]; /* "../../../sunrpc/rpc_clnt/" + 8 hex digits + '\0' */

	if (IS_ERR(client) || !client->cl_debugfs)
		return NULL;

	len = snprintf(target, sizeof(target), "../../../sunrpc/rpc_clnt/%s",
		       client->cl_debugfs->d_name.name);

	if (len >= sizeof(target))
		return NULL;

	return debugfs_create_symlink(name, parent, target);
}

void
nfs_server_debugfs_register(struct nfs_server *server)
{
	char name[26]; /* "../../nfs_client/" + 8 hex digits + '\0' */
	int len;

	if (server->debugfs || !nfs_server_dir)
		return;

	len = snprintf(name, sizeof(name), "%x", server->id);
	if (len >= sizeof(name))
		return;

	server->debugfs = debugfs_create_dir(name, nfs_server_dir);
	if (!server->debugfs)
		return;

	link_rpc_client("rpc_client", server->client, server->debugfs);
	link_rpc_client("rpc_client_acl", server->client_acl, server->debugfs);

	if (server->nfs_client->cl_debugfs) {
		len = snprintf(name, sizeof(name), "../../nfs_client/%s",
			       server->nfs_client->cl_debugfs->d_name.name);
		if (len >= sizeof(name))
			goto out_error;

		if (!debugfs_create_symlink("nfs_client", server->debugfs, name))
			goto out_error;
	}

	return;
out_error:
	debugfs_remove_recursive(server->debugfs);
	server->debugfs = NULL;
}
EXPORT_SYMBOL_GPL(nfs_server_debugfs_register);

void
nfs_server_debugfs_unregister(struct nfs_server* server)
{
	debugfs_remove_recursive(server->debugfs);
	server->debugfs = NULL;
}

void
nfs_client_debugfs_register(struct nfs_client* client)
{
	char name[9]; /* 8 hex digits + '\0' */
	int len;

	if (client->cl_debugfs || !nfs_client_dir)
		return;

	len = snprintf(name, sizeof(name), "%x", client->cl_id);
	if (len >= sizeof(name))
		return;

	client->cl_debugfs = debugfs_create_dir(name, nfs_client_dir);
	if (!client->cl_debugfs)
		return;

	link_rpc_client("cl_rpcclient", client->cl_rpcclient,
			client->cl_debugfs);
}

void
nfs_client_debugfs_unregister(struct nfs_client* client)
{
	debugfs_remove_recursive(client->cl_debugfs);
	client->cl_debugfs = NULL;
}

void
nfs_debugfs_exit(void)
{
	debugfs_remove_recursive(topdir);
	topdir = NULL;
	nfs_client_dir = NULL;
	nfs_server_dir = NULL;
}

void __init
nfs_debugfs_init(void)
{
	topdir = debugfs_create_dir("nfs", NULL);
	if (!topdir)
		return;

	nfs_server_dir = debugfs_create_dir("nfs_server", topdir);
	if (!nfs_server_dir)
		goto out_remove;

	nfs_client_dir = debugfs_create_dir("nfs_client", topdir);
	if (!nfs_client_dir)
		goto out_remove;

	return;
out_remove:
	debugfs_remove_recursive(topdir);
	topdir = NULL;
	nfs_server_dir = NULL;
	nfs_client_dir = NULL;
}


