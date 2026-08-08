#ifndef RET_STATUS_INCLUDED
#define RET_STATUS_INCLUDED

#include <cstdint>
#include <string>
#include <string_view>
#include "status.h"


namespace http {

// Application-level result code returned in the `retCode` field of every response.
// Underlying type is uint32_t since all codes are small non-negative integers.
enum class retCode : unsigned {
    Success                        = 0,     // HTTP 200/201/202
    InvalidJsonOrParams            = 10001, // HTTP 400
    AuthError                      = 10002, // HTTP 401
    Forbidden                      = 10003, // HTTP 403 (no scope/permission)
    NotFound                       = 10005, // HTTP 404
    ConflictDuplicateClientOrderId = 10006, // HTTP 409
    InvalidSimulationState         = 10007, // HTTP 422
    RateLimitExceeded              = 10008, // HTTP 429
    InternalError                  = 10009, // HTTP 500
    InsufficientBalance            = 10010, // HTTP 400
    PriceOrQtyNotAligned           = 10011, // HTTP 400 (not multiple of tickSize/qtyStep)
    PositionOrOrderLimitExceeded   = 10012, // HTTP 422
    UnknownSymbolOrDataset         = 10013, // HTTP 404
    SimulationNotRunning           = 10014, // HTTP 422
    ClientOrderIdAlreadyUsed       = 10015, // HTTP 409
    SessionOrSignatureExpired      = 10016, // HTTP 401
    InvalidSignature               = 10017, // HTTP 401
    ClockSkewExceedsRecvWindow     = 10018, // HTTP 401
    PostOnlyWouldCross             = 10019, // HTTP 422
    RequestBufferOverflow          = 10020, // HTTP 413 (incoming request exceeded receive buffer size)
    MethodNotAllowed               = 10021, // HTTP 405 (verb not supported for this endpoint)
    NotAcceptable                  = 10022, // HTTP 406 (Accept header cannot be satisfied)
    RequestTimeout                 = 10023, // HTTP 408 (client too slow sending the request)
    UnsupportedMediaType           = 10024, // HTTP 415 (Content-Type not accepted, expected application/json)
    MissingContentLength           = 10025, // HTTP 411
    UriTooLong                     = 10026, // HTTP 414
    PreconditionFailed             = 10027, // HTTP 412 (If-Match/If-Unmodified-Since precondition failed)
    ExpectationFailed              = 10028, // HTTP 417
    ResourceGone                   = 10029, // HTTP 410 (resource permanently removed, e.g. expired dataset)
    TooManyOpenOrders              = 10030, // HTTP 429 (per-symbol/account open-order cap hit)
    RequestHeaderFieldsTooLarge    = 10031, // HTTP 431
    UnavailableForLegalReasons     = 10032, // HTTP 451 (symbol/dataset restricted in jurisdiction)
    NotImplemented                 = 10033, // HTTP 501 (endpoint/feature not yet implemented)
    BadGateway                     = 10034, // HTTP 502 (upstream matching engine/market-data feed error)
    ServiceUnavailable             = 10035, // HTTP 503 (server overloaded or in maintenance)
    GatewayTimeout                 = 10036, // HTTP 504 (upstream matching engine/market-data feed timeout)
    InsufficientStorage            = 10037, // HTTP 507
};

// Returns the HTTP status code most commonly associated with a retCode.
// Note: Success (0) is context-dependent — 200 for GET/query, 201 for creation,
// 202 for an accepted async command. Pass the actual HTTP status separately
// when handling Success; the value below is just a sensible default.
constexpr http::status toHttpStatus(retCode code) {
    switch (static_cast<retCode>(code)) {
        case retCode::Success:                         return http::status::ok;
        case retCode::InvalidJsonOrParams:             return http::status::bad_request;
        case retCode::AuthError:                       return http::status::unauthorized;
        case retCode::Forbidden:                       return http::status::forbidden;
        case retCode::NotFound:                        return http::status::not_found;
        case retCode::ConflictDuplicateClientOrderId:  return http::status::conflict;
        case retCode::InvalidSimulationState:          return http::status::unprocessable_entity;
        case retCode::RateLimitExceeded:               return http::status::too_many_requests;
        case retCode::InternalError:                   return http::status::internal_server_error;
        case retCode::InsufficientBalance:             return http::status::bad_request;
        case retCode::PriceOrQtyNotAligned:            return http::status::bad_request;
        case retCode::PositionOrOrderLimitExceeded:    return http::status::unprocessable_entity;
        case retCode::UnknownSymbolOrDataset:          return http::status::not_found;
        case retCode::SimulationNotRunning:            return http::status::unprocessable_entity;
        case retCode::ClientOrderIdAlreadyUsed:        return http::status::conflict;
        case retCode::SessionOrSignatureExpired:       return http::status::unauthorized;
        case retCode::InvalidSignature:                return http::status::unauthorized;
        case retCode::ClockSkewExceedsRecvWindow:      return http::status::unauthorized;
        case retCode::PostOnlyWouldCross:              return http::status::unprocessable_entity;
        case retCode::RequestBufferOverflow:           return http::status::payload_too_large;
        case retCode::MethodNotAllowed:                return http::status::method_not_allowed;
        case retCode::NotAcceptable:                   return http::status::not_acceptable;
        case retCode::RequestTimeout:                  return http::status::request_timeout;
        case retCode::UnsupportedMediaType:            return http::status::unsupported_media_type;
        case retCode::MissingContentLength:            return http::status::length_required;
        case retCode::UriTooLong:                      return http::status::uri_too_long;
        case retCode::PreconditionFailed:              return http::status::precondition_failed;
        case retCode::ExpectationFailed:               return http::status::expectation_failed;
        case retCode::ResourceGone:                    return http::status::gone;
        case retCode::TooManyOpenOrders:               return http::status::too_many_requests;
        case retCode::RequestHeaderFieldsTooLarge:     return http::status::request_header_fields_too_large;
        case retCode::UnavailableForLegalReasons:      return http::status::unavailable_for_legal_reasons;
        case retCode::NotImplemented:                  return http::status::not_implemented;
        case retCode::BadGateway:                      return http::status::bad_gateway;
        case retCode::ServiceUnavailable:              return http::status::service_unavailable;
        case retCode::GatewayTimeout:                  return http::status::gateway_timeout;
        case retCode::InsufficientStorage:             return http::status::insufficient_storage;
        default:
            break;
    }
    return http::status::internal_server_error;
}

// Human-readable description 
constexpr std::string_view retMesg(retCode code) {
    switch (static_cast<retCode>(code)) {
        case retCode::Success:                         return "Success";
        case retCode::InvalidJsonOrParams:             return "Invalid JSON or parameters";
        case retCode::AuthError:                       return "Authorization error";
        case retCode::Forbidden:                       return "Forbidden (insufficient scope)";
        case retCode::NotFound:                        return "Resource not found";
        case retCode::ConflictDuplicateClientOrderId:  return "Conflict or duplicated clientOrderId";
        case retCode::InvalidSimulationState:          return "Command not valid for current simulation state";
        case retCode::RateLimitExceeded:               return "Rate limit exceeded";
        case retCode::InternalError:                   return "Internal error";
        case retCode::InsufficientBalance:             return "Insufficient balance";
        case retCode::PriceOrQtyNotAligned:            return "Price/quantity not aligned with tickSize/qtyStep";
        case retCode::PositionOrOrderLimitExceeded:    return "Position or order size limit exceeded";
        case retCode::UnknownSymbolOrDataset:          return "Unknown symbol or dataset";
        case retCode::SimulationNotRunning:            return "Simulation is not in Running state";
        case retCode::ClientOrderIdAlreadyUsed:        return "clientOrderId already used in this simulation";
        case retCode::SessionOrSignatureExpired:       return "Session/signature expired";
        case retCode::InvalidSignature:                return "Invalid signature";
        case retCode::ClockSkewExceedsRecvWindow:      return "Clock skew exceeds recvWindow";
        case retCode::PostOnlyWouldCross:              return "PostOnly order would cross the book (WouldCross)";
        case retCode::RequestBufferOverflow:           return "Request exceeded receive buffer size";
        case retCode::MethodNotAllowed:                return "HTTP method not allowed for this endpoint";
        case retCode::NotAcceptable:                   return "Requested representation not available (Accept header)";
        case retCode::RequestTimeout:                  return "Client took too long to send the request";
        case retCode::UnsupportedMediaType:            return "Unsupported Content-Type (expected application/json)";
        case retCode::MissingContentLength:            return "Content-Length header is required";
        case retCode::UriTooLong:                      return "Request URI too long";
        case retCode::PreconditionFailed:              return "Precondition failed";
        case retCode::ExpectationFailed:               return "Expectation failed";
        case retCode::ResourceGone:                    return "Resource permanently removed";
        case retCode::TooManyOpenOrders:               return "Too many open orders for this account/symbol";
        case retCode::RequestHeaderFieldsTooLarge:     return "Request header fields too large";
        case retCode::UnavailableForLegalReasons:      return "Unavailable for legal reasons";
        case retCode::NotImplemented:                  return "Not implemented";
        case retCode::BadGateway:                      return "Upstream matching engine/data feed error";
        case retCode::ServiceUnavailable:              return "Service temporarily unavailable";
        case retCode::GatewayTimeout:                  return "Upstream matching engine/data feed timeout";
        case retCode::InsufficientStorage:             return "Insufficient storage";
        default:
            break;
    }
    return "<unknown-retCode>";
}

} // namespace http

#endif