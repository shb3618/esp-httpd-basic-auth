#include "httpd_basic_auth.h"
#include "b64.h"

esp_err_t httpd_resp_send_401_basic_auth_err(httpd_req_t* req) {
	esp_err_t ret = httpd_resp_set_status(req, HTTPD_401);

	if(ret == ESP_OK) {
		ret = httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"User Visible Realm\"");
	}

	return ret;
}

esp_err_t httpd_basic_auth(httpd_req_t* req, const char* username, const char* password) {
	size_t auth_head_len = 1 + httpd_req_get_hdr_value_len(req, "Authorization");

	// Authorization header value needs to start with "Basic "
	// so value of authorization header needs to be at least 7 chars long

	if(auth_head_len <= 1 + 7) {
		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_NOT_FOUND;
	}

	char* auth_head = malloc(auth_head_len);

	if(httpd_req_get_hdr_value_str(req, "Authorization", auth_head, auth_head_len) != ESP_OK) {
		free(auth_head);
		return ESP_ERR_HTTPD_BASIC_AUTH_FAIL_TO_GET_HEADER;
	}

	size_t decoded_len;
	unsigned char* decoded = b64_decode((const unsigned char*) (auth_head + 6), auth_head_len - 6 - 1, &decoded_len);
	free(auth_head);

	if(decoded == NULL) {
		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID;
	}

	char* colonDelimiter = strchr((const char*) decoded, ':');

	if(colonDelimiter == NULL) {
		free(decoded);
		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID;
	}

	size_t head_username_len = colonDelimiter - (const char*) decoded;
	size_t head_password_len = strlen(colonDelimiter + 1);

	if(strlen(username) != head_username_len
		|| strlen(password) != head_password_len
		|| strncmp(username, (const char*) decoded, head_username_len) != 0 
		|| strncmp(password, colonDelimiter + 1, head_password_len) != 0) {
		free(decoded);
		return ESP_ERR_HTTPD_BASIC_AUTH_NOT_AUTHORIZED;
	}

	free(decoded);

	return ESP_OK;
}