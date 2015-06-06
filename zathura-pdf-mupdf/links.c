/* See LICENSE file for license and copyright information */

#include <stdlib.h>

#include "plugin.h"
#include "internal.h"

static bool convert_mupdf_to_zathura_action(fz_link* link, zathura_action_t**
    action);

zathura_error_t
pdf_page_get_links(zathura_page_t* page, zathura_list_t** links)
{
  if (page == NULL || links == NULL) {
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  zathura_error_t error = ZATHURA_ERROR_OK;
  *links = NULL;

  zathura_document_t* document;
  if ((error = zathura_page_get_document(page, &document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_document_t* mupdf_document;
  if ((error = zathura_document_get_data(document, (void**) &mupdf_document)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  mupdf_page_t* mupdf_page;
  if ((error = zathura_page_get_data(page, (void**) &mupdf_page)) != ZATHURA_ERROR_OK) {
    goto error_out;
  }

  fz_link* link = fz_load_links(mupdf_document->ctx, mupdf_page->page);
  for (; link != NULL; link = link->next) {
    /* extract position */
    zathura_rectangle_t position;
    position.p1.x = link->rect.x0;
    position.p2.x = link->rect.x1;
    position.p1.y = link->rect.y0;
    position.p2.y = link->rect.y1;

    zathura_action_t* action;
    if (convert_mupdf_to_zathura_action(link, &action) == false) {
      continue;
    }

    zathura_link_mapping_t* link_mapping = calloc(1, sizeof(zathura_link_mapping_t));
    if (link_mapping == NULL) {
      error = ZATHURA_ERROR_OUT_OF_MEMORY;
      goto error_free;
    }

    link_mapping->position = position;
    link_mapping->action = action;

    *links = zathura_list_append(*links, link_mapping);
  }

  return error;

error_free:

  if (*links != NULL) {
    zathura_list_free_full(*links, free);
    *links = NULL;
  }

error_out:

  return error;
}

static bool
convert_mupdf_to_zathura_action(fz_link* link, zathura_action_t** action)
{
  if (link == NULL || action == NULL) {
    return false;
  }

  switch (link->dest.kind) {
    case FZ_LINK_NONE:
      if (zathura_action_new(action, ZATHURA_ACTION_NONE) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_URI:
      if (zathura_action_new(action, ZATHURA_ACTION_URI) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_GOTO:
      if (zathura_action_new(action, ZATHURA_ACTION_GOTO) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_LAUNCH:
      if (zathura_action_new(action, ZATHURA_ACTION_LAUNCH) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_NAMED:
      if (zathura_action_new(action, ZATHURA_ACTION_NAMED) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    case FZ_LINK_GOTOR:
      if (zathura_action_new(action, ZATHURA_ACTION_GOTO_REMOTE) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
    default:
      if (zathura_action_new(action, ZATHURA_ACTION_UNKNOWN) != ZATHURA_ERROR_OK) {
        return false;
      }
      break;
  }

  return true;
}
